#include "codegen.hpp"
#include "ast/ast.hpp"
#include "logger.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include <map>
#include <memory>

llvm::Value *ast::NumberExpr::codegen(codegen::LLVMCodegenCtx *llctx) {
  return llvm::ConstantFP::get(*llctx->Context, llvm::APFloat(this->val));
}

llvm::Value *ast::VariableExpr::codegen(codegen::LLVMCodegenCtx *llctx) {
  llvm::Value *v = llctx->NamedValues[this->name];
  if (!v)
    ERROR("Unknown variable name: " << this->name);
  return v;
}

llvm::Value *ast::BinaryExpr::codegen(codegen::LLVMCodegenCtx *llctx) {
  llvm::Value *l = this->left->codegen(llctx);
  llvm::Value *r = this->right->codegen(llctx);

  if (!l || !r)
    return nullptr;

  switch (this->op) {
  case ast::OperatorKind::Plus:
    return llctx->Builder->CreateFAdd(l, r, "addtmp");
  case ast::OperatorKind::Minus:
    return llctx->Builder->CreateFSub(l, r, "subtmp");
  case ast::OperatorKind::Asterisk:
    return llctx->Builder->CreateFMul(l, r, "multmp");
  case ast::OperatorKind::LessThan:
    l = llctx->Builder->CreateFCmpULT(l, r, "cmptmp");
    // bool -> double
    return llctx->Builder->CreateUIToFP(
        l, llvm::Type::getDoubleTy(*llctx->Context), "booltmp");
  default:
    ERROR("Invalid binary operator");
    return nullptr;
  }
}

llvm::Value *ast::CallExpr::codegen(codegen::LLVMCodegenCtx *llctx) {
  llvm::Function *calleeF = llctx->Module->getFunction(this->callee);
  if (!calleeF) {
    ERROR("Referenced unknown function: " << this->callee);
    return nullptr;
  }

  // Argument mismatch error
  if (calleeF->arg_size() != this->args.size()) {
    ERROR("Incorrect number of arguments passed");
    return nullptr;
  }

  std::vector<llvm::Value *> argsVec;
  for (uint32_t i = 0, size = this->args.size(); i != size; ++i) {
    argsVec.push_back(this->args[i]->codegen(llctx));
    if (!argsVec.back())
      return nullptr;
  }

  return llctx->Builder->CreateCall(calleeF, argsVec, "calltmp");
}

llvm::Value *ast::IfExpr::codegen(codegen::LLVMCodegenCtx *llctx) {
  llvm::Value *condV = this->Cond->codegen(llctx);
  if (!condV)
    return nullptr;

  // condition -> bool
  condV = llctx->Builder->CreateFCmpONE(
      condV, llvm::ConstantFP::get(*llctx->Context, llvm::APFloat(0.0)),
      "ifcond");

  llvm::Function *function = llctx->Builder->GetInsertBlock()->getParent();
  // create blocks for then & else; insert 'then' at the end
  llvm::BasicBlock *thenBB =
      llvm::BasicBlock::Create(*llctx->Context, "then", function);
  llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*llctx->Context, "else");
  llvm::BasicBlock *mergeBB =
      llvm::BasicBlock::Create(*llctx->Context, "ifcont");

  llctx->Builder->CreateCondBr(condV, thenBB, elseBB);

  // Emit then
  llctx->Builder->SetInsertPoint(thenBB);

  llvm::Value *thenV = this->Then->codegen(llctx);
  if (!thenV)
    return nullptr;
  llctx->Builder->CreateBr(mergeBB);

  // Update because codegen of 'then' can change the current block
  thenBB = llctx->Builder->GetInsertBlock();

  // Emit else
  function->insert(function->end(), elseBB);
  llctx->Builder->SetInsertPoint(elseBB);

  llvm::Value *elseV = this->Else->codegen(llctx);
  if (!elseV)
    return nullptr;
  llctx->Builder->CreateBr(mergeBB);

  // Update because codegen of 'else' can change the current block
  elseBB = llctx->Builder->GetInsertBlock();

  // Emit merge block
  function->insert(function->end(), mergeBB);
  llctx->Builder->SetInsertPoint(mergeBB);
  llvm::PHINode *pn = llctx->Builder->CreatePHI(
      llvm::Type::getDoubleTy(*llctx->Context), 2, "iftmp");
  pn->addIncoming(thenV, thenBB);
  pn->addIncoming(elseV, elseBB);

  return pn;
}

llvm::Value *ast::ForExpr::codegen(codegen::LLVMCodegenCtx *llctx) {
  llvm::Value *startVal = this->Start->codegen(llctx);
  if (!startVal)
    return nullptr;

  llvm::Function *function = llctx->Builder->GetInsertBlock()->getParent();
  llvm::BasicBlock *preheaderBB = llctx->Builder->GetInsertBlock();
  llvm::BasicBlock *loopBB =
      llvm::BasicBlock::Create(*llctx->Context, "loop", function);
  // Go from current block to loop block
  llctx->Builder->CreateBr(loopBB);

  llctx->Builder->SetInsertPoint(loopBB);
  // PHI node with Start entry
  llvm::PHINode *variable = llctx->Builder->CreatePHI(
      llvm::Type::getDoubleTy(*llctx->Context), 2, this->VarName);
  variable->addIncoming(startVal, preheaderBB);

  // Shadow existing variable under the same name but preserve
  llvm::Value *oldVal = llctx->NamedValues[this->VarName];
  llctx->NamedValues[this->VarName] = variable;

  // Emit loop body
  if (!this->Body->codegen(llctx))
    return nullptr;

  // Emit step
  llvm::Value *stepVal = nullptr;
  if (this->Step) {
    stepVal = this->Step->codegen(llctx);
    if (!stepVal)
      return nullptr;
  } else {
    // use 1.0 as default
    stepVal = llvm::ConstantFP::get(*llctx->Context, llvm::APFloat(1.0));
  }

  llvm::Value *nextVar =
      llctx->Builder->CreateFAdd(variable, stepVal, "nextvar");

  // Compute end condition
  llvm::Value *endCond = this->End->codegen(llctx);
  if (!endCond)
    return nullptr;

  // cond -> bool; cmp not-equal to 0
  endCond = llctx->Builder->CreateFCmpONE(
      endCond, llvm::ConstantFP::get(*llctx->Context, llvm::APFloat(0.0)),
      "loopcond");

  // create the "after loop" block and insert it
  llvm::BasicBlock *loopEndBB = llctx->Builder->GetInsertBlock();
  llvm::BasicBlock *afterBB =
      llvm::BasicBlock::Create(*llctx->Context, "afterloop", function);
  llctx->Builder->CreateCondBr(endCond, loopBB, afterBB);
  llctx->Builder->SetInsertPoint(afterBB);

  variable->addIncoming(nextVar, loopEndBB);

  // Restore the unshadowed variable
  if (oldVal)
    llctx->NamedValues[this->VarName] = oldVal;
  else
    llctx->NamedValues.erase(this->VarName);

  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*llctx->Context));
}

llvm::Function *
ast::FunctionPrototype::codegen(codegen::LLVMCodegenCtx *llctx) {
  // Function type, e.g. double(double, double)
  std::vector<llvm::Type *> doubles(this->args.size(),
                                    llvm::Type::getDoubleTy(*llctx->Context));
  llvm::FunctionType *ft = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(*llctx->Context), doubles, false);
  llvm::Function *f = llvm::Function::Create(
      ft, llvm::Function::ExternalLinkage, this->name, llctx->Module.get());

  // Set argument names
  uint32_t idx = 0;
  for (auto &arg : f->args())
    arg.setName(args[idx++]);

  return f;
}

llvm::Function *
ast::FunctionDefinition::codegen(codegen::LLVMCodegenCtx *llctx) {
  // Check if a function prototype already exists
  llvm::Function *function = llctx->Module->getFunction(this->proto->getName());

  if (!function)
    function = this->proto->codegen(llctx);

  if (!function)
    return nullptr;

  if (!function->empty()) {
    ERROR("Function " << this->proto->getName() << " cannot be redefined.");
    return nullptr;
  }

  // Create a new basic block
  llvm::BasicBlock *bb =
      llvm::BasicBlock::Create(*llctx->Context, "entry", function);
  llctx->Builder->SetInsertPoint(bb);

  // Record function arguments in the map
  llctx->NamedValues.clear();
  for (auto &arg : function->args())
    llctx->NamedValues[std::string(arg.getName())] = &arg;

  if (llvm::Value *retVal = this->body->codegen(llctx)) {
    llctx->Builder->CreateRet(retVal);

    // Validate generated code
    llvm::verifyFunction(*function);

    return function;
  }

  // Error processing body -> cleanup
  function->eraseFromParent();
  return nullptr;
}

llvm::Module *ast::CompilationUnit::codegen(codegen::LLVMCodegenCtx *llctx) {
  std::vector<llvm::Function *> fnIRs;
  // Codegen all functions
  for (auto &fn : this->functions)
    fnIRs.push_back(fn->codegen(llctx));

  DEBUG("*** Unoptimised codegen ***");
  if (LoggingLevel == log::debug)
    llctx->Module->print(llvm::outs(), nullptr);

  // Optimise all functions
  for (auto fnIR : fnIRs)
    llctx->FPM->run(*fnIR, *llctx->FAM);

  return &*llctx->Module;
}

void codegen::codegen(ast::CompilationUnit *ast) {
  // Prepare the context struct
  LLVMCodegenCtx llctx;

  // Initialise module
  llctx.Context = std::make_unique<llvm::LLVMContext>();
  llctx.Module = std::make_unique<llvm::Module>(ast->name, *llctx.Context);
  llctx.Builder = std::make_unique<llvm::IRBuilder<>>(*llctx.Context);

  // Crceate pass and analysis managers
  llctx.FPM = std::make_unique<llvm::FunctionPassManager>();
  llctx.LAM = std::make_unique<llvm::LoopAnalysisManager>();
  llctx.FAM = std::make_unique<llvm::FunctionAnalysisManager>();
  llctx.CGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
  llctx.MAM = std::make_unique<llvm::ModuleAnalysisManager>();
  llctx.MPM = std::make_unique<llvm::ModulePassManager>();
  llctx.PIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
  llctx.SI =
      std::make_unique<llvm::StandardInstrumentations>(*llctx.Context, true);

  llctx.SI->registerCallbacks(*llctx.PIC, llctx.MAM.get());

  // Add function transform passes
  llctx.FPM->addPass(llvm::InstCombinePass());
  llctx.FPM->addPass(llvm::ReassociatePass());
  llctx.FPM->addPass(llvm::GVNPass());
  llctx.FPM->addPass(llvm::SimplifyCFGPass());

  llvm::PassBuilder pb;
  pb.registerModuleAnalyses(*llctx.MAM);
  pb.registerFunctionAnalyses(*llctx.FAM);
  pb.crossRegisterProxies(*llctx.LAM, *llctx.FAM, *llctx.CGAM, *llctx.MAM);

  DEBUG("*** Starting codegen ***");
  llvm::Module *module = ast->codegen(&llctx);

  // Run optimisations on the module
  llctx.MPM->run(*module, *llctx.MAM);

  DEBUG("*** Optimised codegen ***");
  if (LoggingLevel == log::debug)
    module->print(llvm::outs(), nullptr);
}

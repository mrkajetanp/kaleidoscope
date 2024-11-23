#include "codegen.hpp"
#include "ast/ast.hpp"
#include "easylogging++.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include <map>
#include <memory>

static std::unique_ptr<llvm::LLVMContext> LLContext;
static std::unique_ptr<llvm::IRBuilder<>> LLBuilder;
static std::unique_ptr<llvm::Module> LLModule;
static std::map<std::string, llvm::Value *> LLNamedValues;

static std::unique_ptr<llvm::FunctionPassManager> LLFPM;
static std::unique_ptr<llvm::LoopAnalysisManager> LLLAM;
static std::unique_ptr<llvm::FunctionAnalysisManager> LLFAM;
static std::unique_ptr<llvm::CGSCCAnalysisManager> LLCGAM;
static std::unique_ptr<llvm::ModuleAnalysisManager> LLMAM;
static std::unique_ptr<llvm::PassInstrumentationCallbacks> LLPIC;
static std::unique_ptr<llvm::StandardInstrumentations> LLSI;

llvm::Value *ast::NumberExpr::codegen() {
  return llvm::ConstantFP::get(*LLContext, llvm::APFloat(this->val));
}

llvm::Value *ast::VariableExpr::codegen() {
  llvm::Value *v = LLNamedValues[this->name];
  if (!v)
    LOG(ERROR) << "Unknown variable name: " << this->name;
  return v;
}

llvm::Value *ast::BinaryExpr::codegen() {
  llvm::Value *l = this->left->codegen();
  llvm::Value *r = this->right->codegen();

  if (!l || !r)
    return nullptr;

  switch (this->op) {
  case ast::OperatorKind::Plus:
    return LLBuilder->CreateFAdd(l, r, "addtmp");
  case ast::OperatorKind::Minus:
    return LLBuilder->CreateFSub(l, r, "subtmp");
  case ast::OperatorKind::Asterisk:
    return LLBuilder->CreateFMul(l, r, "multmp");
  case ast::OperatorKind::LessThan:
    l = LLBuilder->CreateFCmpULT(l, r, "cmptmp");
    // bool -> double
    return LLBuilder->CreateUIToFP(l, llvm::Type::getDoubleTy(*LLContext),
                                   "booltmp");
  default:
    LOG(ERROR) << "Invalid binary operator";
    return nullptr;
  }
}

llvm::Value *ast::CallExpr::codegen() {
  llvm::Function *calleeF = LLModule->getFunction(this->callee);
  if (!calleeF) {
    LOG(ERROR) << "Referenced unknown function: " << this->callee;
    return nullptr;
  }

  // Argument mismatch error
  if (calleeF->arg_size() != this->args.size()) {
    LOG(ERROR) << "Incorrect number of arguments passed";
    return nullptr;
  }

  std::vector<llvm::Value *> argsVec;
  for (uint32_t i = 0, size = this->args.size(); i != size; ++i) {
    argsVec.push_back(this->args[i]->codegen());
    if (!argsVec.back())
      return nullptr;
  }

  return LLBuilder->CreateCall(calleeF, argsVec, "calltmp");
}

llvm::Function *ast::FunctionPrototype::codegen() {
  // Function type, e.g. double(double, double)
  std::vector<llvm::Type *> doubles(this->args.size(),
                                    llvm::Type::getDoubleTy(*LLContext));
  llvm::FunctionType *ft = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(*LLContext), doubles, false);
  llvm::Function *f = llvm::Function::Create(
      ft, llvm::Function::ExternalLinkage, this->name, LLModule.get());

  // Set argument names
  uint32_t idx = 0;
  for (auto &arg : f->args())
    arg.setName(args[idx++]);

  return f;
}

llvm::Function *ast::FunctionDefinition::codegen() {
  // Check if a function prototype already exists
  llvm::Function *function = LLModule->getFunction(this->proto->getName());

  if (!function)
    function = this->proto->codegen();

  if (!function)
    return nullptr;

  if (!function->empty()) {
    LOG(ERROR) << "Function " << this->proto->getName()
               << " cannot be redefined.";
    return nullptr;
  }

  // Create a new basic block
  llvm::BasicBlock *bb =
      llvm::BasicBlock::Create(*LLContext, "entry", function);
  LLBuilder->SetInsertPoint(bb);

  // Record function arguments in the map
  LLNamedValues.clear();
  for (auto &arg : function->args())
    LLNamedValues[std::string(arg.getName())] = &arg;

  if (llvm::Value *retVal = this->body->codegen()) {
    LLBuilder->CreateRet(retVal);

    // Validate generated code
    llvm::verifyFunction(*function);

    // Optimise
    LLFPM->run(*function, *LLFAM);

    return function;
  }

  // Error processing body -> cleanup
  function->eraseFromParent();
  return nullptr;
}

void codegen::codegen(ast::CompilationUnit *ast) {
  // Initialise module
  LLContext = std::make_unique<llvm::LLVMContext>();
  LLModule = std::make_unique<llvm::Module>("kaleidoscope-module", *LLContext);
  LLBuilder = std::make_unique<llvm::IRBuilder<>>(*LLContext);

  // Crceate pass and analysis managers
  LLFPM = std::make_unique<llvm::FunctionPassManager>();
  LLLAM = std::make_unique<llvm::LoopAnalysisManager>();
  LLFAM = std::make_unique<llvm::FunctionAnalysisManager>();
  LLCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
  LLMAM = std::make_unique<llvm::ModuleAnalysisManager>();
  LLPIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
  LLSI = std::make_unique<llvm::StandardInstrumentations>(*LLContext, true);

  LLSI->registerCallbacks(*LLPIC, LLMAM.get());

  // Add transform passes
  LLFPM->addPass(llvm::InstCombinePass());
  LLFPM->addPass(llvm::ReassociatePass());
  LLFPM->addPass(llvm::GVNPass());
  LLFPM->addPass(llvm::SimplifyCFGPass());

  llvm::PassBuilder pb;
  pb.registerModuleAnalyses(*LLMAM);
  pb.registerFunctionAnalyses(*LLFAM);
  pb.crossRegisterProxies(*LLLAM, *LLFAM, *LLCGAM, *LLMAM);

  VLOG(5) << "Starting codegen";
  for (auto &fn : ast->functions) {
    if (auto *fnIR = fn->codegen()) {
      fnIR->print(llvm::outs());
      llvm::outs() << "\n";
    }
  }
}

#include "codegen.hpp"
#include "ast/ast.hpp"
#include "easylogging++.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include <map>

static std::unique_ptr<llvm::LLVMContext> LLContext;
static std::unique_ptr<llvm::IRBuilder<>> LLBuilder;
static std::unique_ptr<llvm::Module> LLModule;
static std::map<std::string, llvm::Value *> LLNamedValues;

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

  VLOG(5) << "Starting codegen";
  for (auto &fn : ast->functions) {
    if (auto *fnIR = fn->codegen()) {
      fnIR->print(llvm::outs());
      llvm::outs() << "\n";
    }
  }
}

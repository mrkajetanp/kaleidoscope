#ifndef AST_H_
#define AST_H_

#include "codegen.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ast {

class Expr {
public:
  virtual std::string tree_format(uint32_t indent_level) = 0;
  virtual ~Expr() = default;
  virtual llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) = 0;
};

class NumberExpr : public Expr {
  double val;

public:
  NumberExpr(double val) : val(val) {}
  llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) override;
  std::string tree_format(uint32_t indent_level) override;
};

class VariableExpr : public Expr {
  std::string name;

public:
  VariableExpr(const std::string &name) : name(name) {}
  llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) override;
  std::string tree_format(uint32_t indent_level) override;
};

enum class OperatorKind {
  Plus,
  Minus,
  LessThan,
  GreaterThan,
  Asterisk,
};

class BinaryExpr : public Expr {
  OperatorKind op;
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;

public:
  BinaryExpr(OperatorKind op, std::unique_ptr<Expr> left,
             std::unique_ptr<Expr> right)
      : op(op), left(std::move(left)), right(std::move(right)) {}

  llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) override;
  std::string tree_format(uint32_t indent_level) override;
};

class CallExpr : public Expr {
  std::string callee;
  std::vector<std::unique_ptr<Expr>> args;

public:
  CallExpr(const std::string &callee, std::vector<std::unique_ptr<Expr>> args)
      : callee(callee), args(std::move(args)) {}

  llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) override;
  std::string tree_format(uint32_t indent_level) override;
};

class IfExpr : public Expr {
  std::unique_ptr<ast::Expr> Cond;
  std::unique_ptr<ast::Expr> Then;
  std::unique_ptr<ast::Expr> Else;

public:
  IfExpr(std::unique_ptr<Expr> Cond, std::unique_ptr<Expr> Then,
         std::unique_ptr<Expr> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) override;
  std::string tree_format(uint32_t indent_level) override;
};

class ForExpr : public Expr {
  std::string VarName;
  std::unique_ptr<Expr> Start;
  std::unique_ptr<Expr> End;
  std::unique_ptr<Expr> Step;
  std::unique_ptr<Expr> Body;

public:
  ForExpr(const std::string &VarName, std::unique_ptr<Expr> Start,
          std::unique_ptr<Expr> End, std::unique_ptr<Expr> Step,
          std::unique_ptr<Expr> Body)
      : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}

  llvm::Value *codegen(codegen::LLVMCodegenCtx *llctx) override;
  std::string tree_format(uint32_t indent_level) override;
};

class FunctionPrototype {
public:
  std::string name;
  std::vector<std::string> args;

  FunctionPrototype(const std::string &name, std::vector<std::string> args)
      : name(name), args(std::move(args)) {}

  const std::string &getName() const { return this->name; }
  llvm::Function *codegen(codegen::LLVMCodegenCtx *llctx);
};

class FunctionDefinition {
public:
  std::unique_ptr<FunctionPrototype> proto;
  std::unique_ptr<Expr> body;

  FunctionDefinition(std::unique_ptr<FunctionPrototype> proto,
                     std::unique_ptr<Expr> body)
      : proto(std::move(proto)), body(std::move(body)) {}

  llvm::Function *codegen(codegen::LLVMCodegenCtx *llctx);
};

class CompilationUnit {
public:
  std::string name;
  std::vector<std::unique_ptr<ast::FunctionDefinition>> functions;

  CompilationUnit(
      std::string name,
      std::vector<std::unique_ptr<ast::FunctionDefinition>> functions)
      : name(name), functions(std::move(functions)) {}

  llvm::Module *codegen(codegen::LLVMCodegenCtx *llctx);
};

} // namespace ast

#endif // AST_H_

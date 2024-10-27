#ifndef AST_H_
#define AST_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ast {

class Expr {
public:
  virtual std::string tree_format(uint32_t indent_level) = 0;
  virtual ~Expr() = default;
};

class NumberExpr : public Expr {
  double val;

public:
  NumberExpr(double val) : val(val) {}
  std::string tree_format(uint32_t indent_level) override;
};

class VariableExpr : public Expr {
  std::string name;

public:
  VariableExpr(const std::string &name) : name(name) {}
  std::string tree_format(uint32_t indent_level) override;
};

enum class OperatorKind {
  Plus,
  Minus,
  LessThan,
  GreaterThan,
  Asterisk,
  Invalid,
};

class BinaryExpr : public Expr {
  OperatorKind op;
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;

public:
  BinaryExpr(OperatorKind op, std::unique_ptr<Expr> left,
             std::unique_ptr<Expr> right)
      : op(op), left(std::move(left)), right(std::move(right)) {}

  std::string tree_format(uint32_t indent_level) override;
};

class CallExpr : public Expr {
  std::string callee;
  std::vector<std::unique_ptr<Expr>> args;

public:
  CallExpr(const std::string &callee, std::vector<std::unique_ptr<Expr>> args)
      : callee(callee), args(std::move(args)) {}
  std::string tree_format(uint32_t indent_level) override;
};

class FunctionPrototype {
public:
  std::string name;
  std::vector<std::string> args;

  FunctionPrototype(const std::string &name, std::vector<std::string> args)
      : name(name), args(std::move(args)) {}

  const std::string &getName() const { return this->name; }
};

class FunctionDefinition {
public:
  std::unique_ptr<FunctionPrototype> proto;
  std::unique_ptr<Expr> body;

  FunctionDefinition(std::unique_ptr<FunctionPrototype> proto,
                     std::unique_ptr<Expr> body)
      : proto(std::move(proto)), body(std::move(body)) {}
};
} // namespace ast

#endif // AST_H_

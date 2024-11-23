#include "ast/printer.hpp"
#include "ast/ast.hpp"
#include <sstream>

std::string ast::BinaryExpr::tree_format(uint32_t indent_level) {
  std::string indent = "";
  for (int i = 0; i < indent_level; ++i)
    indent += INDENT;
  std::stringstream result;
  result << indent << "BinaryExpr" << '\n';
  indent += INDENT;

  result << indent << std::format("Op: {}\n", op);
  result << indent
         << std::format("Left:\n{}", left->tree_format(indent_level + 2));
  result << indent
         << std::format("Right:\n{}", right->tree_format(indent_level + 2));
  return result.str();
}

std::string ast::CallExpr::tree_format(uint32_t indent_level) {
  std::string indent = "";
  for (int i = 0; i < indent_level; ++i)
    indent += INDENT;
  std::stringstream result;
  result << indent << "CallExpr" << '\n';
  indent += INDENT;
  result << indent << "Callee: " << this->callee << '\n';
  result << indent << "Args: " << '\n';
  for (auto &arg : this->args)
    result << arg->tree_format(indent_level + 2);
  return result.str();
}

std::string ast::NumberExpr::tree_format(uint32_t indent_level) {
  std::string indent = "";
  for (int i = 0; i < indent_level; ++i)
    indent += INDENT;
  return std::format("{}NumberExpr: {}\n", indent, this->val);
}

std::string ast::VariableExpr::tree_format(uint32_t indent_level) {
  std::string indent = "";
  for (int i = 0; i < indent_level; ++i)
    indent += INDENT;
  return std::format("{}VariableExpr: {}\n", indent, this->name);
}

std::string ast::IfExpr::tree_format(uint32_t indent_level) {
  std::string indent = "";
  for (int i = 0; i < indent_level; ++i)
    indent += INDENT;
  std::stringstream result;
  result << indent << "IfExpr:" << '\n';
  indent += INDENT;
  result << indent << "Cond:" << '\n'
         << this->Cond->tree_format(indent_level + 2);
  result << indent << "Then:" << '\n'
         << this->Then->tree_format(indent_level + 2);
  result << indent << "Else:" << '\n'
         << this->Else->tree_format(indent_level + 2);
  return result.str();
}

std::string ast::ForExpr::tree_format(uint32_t indent_level) {
  std::string indent = "";
  for (int i = 0; i < indent_level; ++i)
    indent += INDENT;
  std::stringstream result;

  result << indent << "ForExpr:" << '\n';
  indent += INDENT;
  result << indent << "VarName: " << this->VarName << '\n';
  result << indent << "Start:" << '\n'
         << this->Start->tree_format(indent_level + 2);
  result << indent << "End:" << '\n'
         << this->End->tree_format(indent_level + 2);
  result << indent << "Step:" << '\n'
         << this->Step->tree_format(indent_level + 2);
  result << indent << "Body:" << '\n'
         << this->Body->tree_format(indent_level + 2);

  return result.str();
}

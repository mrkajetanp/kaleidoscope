#ifndef AST_PRINTER_H_
#define AST_PRINTER_H_

#include "ast/ast.hpp"

#define INDENT "  "

template <> struct std::formatter<ast::FunctionPrototype> {
  int indent_level = 0;

  constexpr auto parse(std::format_parse_context &ctx) {
    auto pos = ctx.begin();
    while (pos != ctx.end() && *pos != '}') {
      if (*pos >= '0' && *pos <= '9')
        indent_level = *pos - '0';
      ++pos;
    }
    return pos;
  }

  auto format(const ast::FunctionPrototype &fn,
              std::format_context &ctx) const {
    std::string indent = "";

    for (int i = 0; i < this->indent_level; ++i)
      indent += INDENT;

    std::string name = fn.getName();
    if (name.empty())
      name = "[Anonymous]";
    std::format_to(ctx.out(), "{}Name: {}", indent, name);

    if (!fn.args.empty()) {
      std::format_to(ctx.out(), "\n{}Args: ", indent);
      for (auto &arg : fn.args)
        std::format_to(ctx.out(), "{} ", arg);
    }
    return std::format_to(ctx.out(), "");
  }
};

template <> struct std::formatter<ast::FunctionDefinition> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
  auto format(const ast::FunctionDefinition &fn,
              std::format_context &ctx) const {
    std::format_to(ctx.out(), "FunctionDefinition\n");
    std::string proto = std::format("{}Proto:\n{:2}", INDENT, *fn.proto);
    std::string body = fn.body->tree_format(2);
    return std::format_to(ctx.out(), "{}\n{}Body:\n{}", proto, INDENT, body);
  }
};

template <> struct std::formatter<ast::CompilationUnit> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
  auto format(const ast::CompilationUnit &cu, std::format_context &ctx) const {
    std::format_to(ctx.out(), "CompilationUnit\n\n");
    for (auto &fn : cu.functions)
      std::format_to(ctx.out(), "{}\n", *fn);
    return std::format_to(ctx.out(), "");
  }
};

#define OP_KIND_FORMAT_CASE(name, symbol)                                      \
  case ast::OperatorKind::name:                                                \
    result = symbol;                                                           \
    break;

template <> struct std::formatter<ast::OperatorKind> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
  auto format(const ast::OperatorKind &op, std::format_context &ctx) const {
    std::string result;
    switch (op) {
      OP_KIND_FORMAT_CASE(Plus, "+")
      OP_KIND_FORMAT_CASE(Minus, "-")
      OP_KIND_FORMAT_CASE(Asterisk, "*")
      OP_KIND_FORMAT_CASE(LessThan, "<")
      OP_KIND_FORMAT_CASE(GreaterThan, "<")
    }
    return std::format_to(ctx.out(), "{}", result);
  }
};

#undef OP_KIND_FORMAT_CASE

#endif // AST_PRINTER_H_

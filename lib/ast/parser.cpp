#include "ast/parser.hpp"
#include "ast/ast.hpp"
#include "lexer.hpp"
#include <iostream>
#include <memory>
#include <print>

static void debugPrintTokens(std::deque<Token> &tokens) {
  for (int i = 0; i < 5 && i < tokens.size(); ++i)
    std::print("{} ", tokens[i].format());
  std::println();
}

namespace parser {

static std::unique_ptr<ast::Expr> parseExpr(std::deque<Token> &tokens);
static std::unique_ptr<ast::FunctionDefinition>
parseFunctionDefinition(std::deque<Token> &tokens);
static std::unique_ptr<ast::FunctionDefinition>
parseTopLevelExpr(std::deque<Token> &tokens);
static std::unique_ptr<ast::FunctionPrototype>
parseExtern(std::deque<Token> &tokens);
std::optional<ast::OperatorKind> tokenToBinaryOperator(Token token);

ast::CompilationUnit parse(std::deque<Token> &tokens) {
  auto functions = std::vector<std::unique_ptr<ast::FunctionDefinition>>();

  while (tokens.size() > 0) {
    auto token = tokens.front();
    switch (token.getKind()) {
    case TokenKind::Def:
      functions.push_back(parseFunctionDefinition(tokens));
      break;
    default:
      functions.push_back(parseTopLevelExpr(tokens));
    }
  }
  return ast::CompilationUnit(std::move(functions));
}

static std::unique_ptr<ast::Expr> parseNumberExpr(std::deque<Token> &tokens) {
  auto token = tokens.front();
  tokens.pop_front();
  auto data = std::get<double>(*token.getData());
  auto result = std::make_unique<ast::NumberExpr>(data);
  return std::move(result);
}

static std::unique_ptr<ast::Expr> parseParenExpr(std::deque<Token> &tokens) {
  std::println("Parsing paren expr");
  tokens.pop_front();
  auto val = parseExpr(tokens);
  if (!val)
    return nullptr;

  auto token = tokens.front();
  if (token.getKind() != TokenKind::ParenClose) {
    std::println(std::cerr, "Expected ')'");
    return nullptr;
  }
  tokens.pop_front();
  return std::move(val);
}

static std::unique_ptr<ast::Expr>
parseIdentifierExpr(std::deque<Token> &tokens) {
  auto token = tokens.front();
  std::string idName = std::get<std::string>(*token.getData());
  tokens.pop_front();

  token = tokens.front();
  // Variable reference
  if (token.getKind() != TokenKind::ParenOpen)
    return std::make_unique<ast::VariableExpr>(idName);

  // Function call
  tokens.pop_front();
  std::println("parsing function call here");
  token = tokens.front();
  std::vector<std::unique_ptr<ast::Expr>> args;
  if (token.getKind() != TokenKind::ParenClose) {
    while (true) {
      if (auto arg = parseExpr(tokens))
        args.push_back(std::move(arg));
      else
        return nullptr;

      std::println("pushed {} args", args.size());
      debugPrintTokens(tokens);

      token = tokens.front();
      if (token.getKind() == TokenKind::ParenClose)
        break;

      if (token.getKind() != TokenKind::Comma) {
        std::println(std::cerr, "Expected ')' or ',' in argument list");
        return nullptr;
      }

      tokens.pop_front();
    }
  }

  // Pop the ')'
  tokens.pop_front();

  return std::make_unique<ast::CallExpr>(idName, std::move(args));
}

static std::unique_ptr<ast::Expr> parsePrimary(std::deque<Token> &tokens) {
  auto token = tokens.front();
  switch (token.getKind()) {
  case TokenKind::Identifier:
    return parseIdentifierExpr(tokens);
  case TokenKind::Number:
    return parseNumberExpr(tokens);
  case TokenKind::ParenOpen:
    return parseParenExpr(tokens);
  default:
    std::println(std::cerr,
                 "Unknown token when parsing a primary expression {}",
                 token.format());
    return nullptr;
  }
}

static std::unique_ptr<ast::Expr>
parseBinOpRhs(std::deque<Token> &tokens, int precedence,
              std::unique_ptr<ast::Expr> lhs) {
  while (true) {
    auto token = tokens.front();
    int newPrecedence = token.precedence();
    if (newPrecedence < precedence)
      return lhs;

    auto op = tokens.front();
    tokens.pop_front();
    auto rhs = parsePrimary(tokens);
    if (!rhs)
      return nullptr;

    token = tokens.front();
    if (newPrecedence < token.precedence()) {
      rhs = parseBinOpRhs(tokens, newPrecedence + 1, std::move(rhs));
      if (!rhs)
        return nullptr;
    }

    auto binop = tokenToBinaryOperator(op);
    if (!binop.has_value()) {
      std::println(std::cerr, "Invalid binary operator {}", token.format());
      return nullptr;
    }
    // merge lhs & rhs
    lhs = std::make_unique<ast::BinaryExpr>(binop.value(), std::move(lhs),
                                            std::move(rhs));
  }
}

static std::unique_ptr<ast::Expr> parseExpr(std::deque<Token> &tokens) {
  std::println("parsing expr");
  debugPrintTokens(tokens);
  auto lhs = parsePrimary(tokens);
  if (!lhs)
    return nullptr;

  return parseBinOpRhs(tokens, 0, std::move(lhs));
}

static std::unique_ptr<ast::FunctionPrototype>
parseFunctionPrototype(std::deque<Token> &tokens) {
  std::println("Parsing function proto");
  debugPrintTokens(tokens);
  auto token = tokens.front();
  if (token.getKind() != TokenKind::Identifier) {
    std::println(std::cerr, "Expected function name in prototype");
    return nullptr;
  }

  std::string functionName = std::get<std::string>(*token.getData());
  tokens.pop_front();
  token = tokens.front();

  if (token.getKind() != TokenKind::ParenOpen) {
    std::println(std::cerr, "Expected '(' in prototype");
    return nullptr;
  }

  tokens.pop_front();
  token = tokens.front();
  std::vector<std::string> argNames;
  while (token.getKind() == TokenKind::Identifier) {
    argNames.push_back(std::get<std::string>(*token.getData()));
    tokens.pop_front();
    if (tokens.front().getKind() == TokenKind::Comma) {
      tokens.pop_front();
    }
    token = tokens.front();
  }

  if (token.getKind() != TokenKind::ParenClose) {
    std::println(std::cerr, "Expected ')' in prototype");
    return nullptr;
  }

  tokens.pop_front();

  std::println("got {} args for {}", argNames.size(), functionName);

  return std::make_unique<ast::FunctionPrototype>(functionName,
                                                  std::move(argNames));
}

static std::unique_ptr<ast::FunctionDefinition>
parseFunctionDefinition(std::deque<Token> &tokens) {
  std::println("Parsing function def");
  debugPrintTokens(tokens);
  // Consume 'def'
  tokens.pop_front();
  auto proto = parseFunctionPrototype(tokens);
  if (!proto)
    return nullptr;

  std::println("parsing function body");
  if (auto expr = parseExpr(tokens))
    return std::make_unique<ast::FunctionDefinition>(std::move(proto),
                                                     std::move(expr));
  return nullptr;
}

static std::unique_ptr<ast::FunctionPrototype>
parseExtern(std::deque<Token> &tokens) {
  tokens.pop_front();
  return parseFunctionPrototype(tokens);
}

static std::unique_ptr<ast::FunctionDefinition>
parseTopLevelExpr(std::deque<Token> &tokens) {
  if (auto expr = parseExpr(tokens)) {
    // anonymous prototype
    auto proto = std::make_unique<ast::FunctionPrototype>(
        "", std::vector<std::string>());
    return std::make_unique<ast::FunctionDefinition>(std::move(proto),
                                                     std::move(expr));
  }

  return nullptr;
}

// *** helpers *** //

#define TOKEN_BINOP_CASE(kind)                                                 \
  case TokenKind::kind:                                                        \
    return ast::OperatorKind::kind;

std::optional<ast::OperatorKind> tokenToBinaryOperator(Token token) {
  switch (token.getKind()) {
    TOKEN_BINOP_CASE(Plus)
    TOKEN_BINOP_CASE(Minus)
    TOKEN_BINOP_CASE(Asterisk)
    TOKEN_BINOP_CASE(LessThan)
  default:
    std::println(std::cerr, "Invalid binary operator {}", token.format());
    return {};
  }
}

#undef TOKEN_BINOP_CASE

} // namespace parser

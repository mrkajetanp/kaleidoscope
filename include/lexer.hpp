#ifndef LEXER_H_
#define LEXER_H_

#include "llvm/Support/MemoryBuffer.h"
#include <deque>
#include <optional>
#include <string>
#include <variant>

enum class TokenKind {
  // Symbols
  ParenOpen,
  ParenClose,
  LessThan,
  Minus,
  Plus,
  Asterisk,
  Comma,
  Semicolon,
  Assignment,
  // Keywords
  Def,
  Extern,
  If,
  Then,
  Else,
  For,
  In,
  // Primary
  Identifier,
  Number,
};

using OptionalTokenData = std::optional<std::variant<std::string, double>>;

class Token {
public:
  TokenKind kind;
  OptionalTokenData data;

  Token(TokenKind kind) : kind(kind), data(std::nullopt) {}
  Token(TokenKind kind, OptionalTokenData data) : kind(kind), data(data) {}
  Token(std::string str);

  static std::optional<Token> from_symbol(char symbol);

  TokenKind getKind() { return this->kind; }
  OptionalTokenData &getData() { return this->data; }
  int precedence();
};

using TokenizeResult = std::variant<std::deque<Token>, std::string>;

TokenizeResult tokenize(const llvm::MemoryBuffer *buffer);

#define TOKEN_FORMAT_CASE(kind)                                                \
  case TokenKind::kind:                                                        \
    result = #kind;                                                            \
    break;

template <> struct std::formatter<Token> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
  auto format(const Token &token, std::format_context &ctx) const {
    std::string result;
    switch (token.kind) {
      TOKEN_FORMAT_CASE(ParenOpen)
      TOKEN_FORMAT_CASE(ParenClose)
      TOKEN_FORMAT_CASE(LessThan)
      TOKEN_FORMAT_CASE(Minus)
      TOKEN_FORMAT_CASE(Plus)
      TOKEN_FORMAT_CASE(Asterisk)
      TOKEN_FORMAT_CASE(Comma)
      TOKEN_FORMAT_CASE(Semicolon)
      TOKEN_FORMAT_CASE(Assignment)
      TOKEN_FORMAT_CASE(Def)
      TOKEN_FORMAT_CASE(Extern)
      TOKEN_FORMAT_CASE(If)
      TOKEN_FORMAT_CASE(Then)
      TOKEN_FORMAT_CASE(Else)
      TOKEN_FORMAT_CASE(For)
      TOKEN_FORMAT_CASE(In)
    case TokenKind::Identifier:
      result = "Identifier(" + std::get<std::string>(token.data.value()) + ")";
      break;
    case TokenKind::Number:
      result = "Number(" +
               std::to_string(std::get<double>(token.data.value())) + ")";
      break;
    }
    return std::format_to(ctx.out(), "{}", result);
  }
};

#undef TOKEN_FORMAT_CASE
#endif // LEXER_H_

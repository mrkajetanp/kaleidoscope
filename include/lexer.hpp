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
  // Keywords
  Def,
  Extern,
  If,
  Then,
  Else,
  // Primary
  Identifier,
  Number,
};

using OptionalTokenData = std::optional<std::variant<std::string, double>>;

class Token {
  TokenKind kind;
  OptionalTokenData data;

public:
  Token(TokenKind kind) : kind(kind), data(std::nullopt) {}
  Token(TokenKind kind, OptionalTokenData data) : kind(kind), data(data) {}
  Token(std::string str);

  std::string format();
  static std::optional<Token> from_symbol(char symbol);

  TokenKind getKind() { return this->kind; }
  OptionalTokenData &getData() { return this->data; }
  int precedence();
};

using TokenizeResult = std::variant<std::deque<Token>, std::string>;

TokenizeResult tokenize(const llvm::MemoryBuffer *buffer);

#endif // LEXER_H_

#ifndef LEXER_HPP_
#define LEXER_HPP_

#include "llvm/Support/MemoryBuffer.h"
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TokenKind {
  // Keywords
  Def,
  Extern,
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
};

std::vector<Token> tokenize(const llvm::MemoryBuffer *buffer);

#endif // LEXER_HPP_

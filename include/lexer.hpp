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
  Token(TokenKind kind) {
    this->kind = kind;
    this->data = std::nullopt;
  }

  Token(TokenKind kind, OptionalTokenData data) {
    this->kind = kind;
    this->data = data;
  }

  std::string format() {
    std::string result;
    switch (this->kind) {
    case TokenKind::Def:
      result = "Def";
      break;
    case TokenKind::Extern:
      result = "Extern";
      break;
    case TokenKind::Identifier:
      result = "Identifier(" + std::get<std::string>(this->data.value()) + ")";
      break;
    case TokenKind::Number:
      result = "Number(" +
               std::to_string(std::get<double>(this->data.value())) + ")";
      break;
    }
    return result;
  }
};

std::vector<Token> tokenize(const llvm::MemoryBuffer *buffer);

#endif // LEXER_HPP_

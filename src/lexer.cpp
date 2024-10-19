#include "lexer.hpp"
#include "llvm/Support/MemoryBuffer.h"
#include <cctype>
#include <print>

std::vector<Token> tokenize(const llvm::MemoryBuffer *buffer) {
  std::vector<Token> result{};

  for (char *pos = (char *)buffer->getBufferStart();
       pos < buffer->getBufferEnd(); pos++) {
    while (std::isspace(*pos))
      ++pos;

    // Handle identifiers
    if (std::isalpha(*pos)) {
      std::string identifier;

      while (std::isalnum(*pos))
        identifier += *pos++;

      result.push_back(Token(identifier));
    }

    // Handle numbers
    if (isdigit(*pos) || *pos == '.') {
      std::string number;
      do {
        number += *pos++;
      } while (isdigit(*pos) || *pos == '.');
      double value = std::stod(number);
      result.push_back(Token{TokenKind::Number, OptionalTokenData(value)});
    }

    // Handle comments
    if (*pos == '#') {
      do
        pos++;
      while (*pos != '\0' && *pos != '\n' && *pos != '\r');
    }
  }

  return result;
}

Token::Token(std::string str) {
  this->data = std::nullopt;

  if (str == "def")
    this->kind = TokenKind::Def;
  else if (str == "extern")
    this->kind = TokenKind::Extern;
  else {
    this->kind = TokenKind::Identifier;
    this->data = OptionalTokenData(str);
  }
}

std::string Token::format() {
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
    result =
        "Number(" + std::to_string(std::get<double>(this->data.value())) + ")";
    break;
  }
  return result;
}

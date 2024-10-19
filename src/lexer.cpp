#include "lexer.hpp"
#include "llvm/Support/MemoryBuffer.h"
#include <cctype>
#include <print>

TokenizeResult tokenize(const llvm::MemoryBuffer *buffer) {
  std::vector<Token> result{};

  char *pos = (char *)buffer->getBufferStart();
  const char *end = buffer->getBufferEnd();
  while (pos < end) {
    while (std::isspace(*pos))
      ++pos;

    // Handle EOF
    if (*pos == '\0')
      break;

    // std::println("startpos {}", *pos);

    // Handle symbols
    std::optional<Token> symbol_token = Token::from_symbol(*pos);
    if (symbol_token.has_value()) {
      result.push_back(symbol_token.value());
      // std::println("adding {}", symbol_token.value().format());
      pos++;
      continue;
    }

    // Handle identifiers
    if (std::isalpha(*pos)) {
      std::string identifier;

      while (std::isalnum(*pos))
        identifier += *pos++;

      result.push_back(Token(identifier));
      // std::println("adding {}", identifier);
      continue;
    }

    // Handle numbers
    if (isdigit(*pos) || *pos == '.') {
      std::string number;
      do {
        number += *pos++;
      } while (isdigit(*pos) || *pos == '.');
      double value = std::stod(number);
      result.push_back(Token{TokenKind::Number, OptionalTokenData(value)});
      // std::println("adding {} pos {}", value, *pos);
      continue;
    }

    // Handle comments
    if (*pos == '#') {
      do
        pos++;
      while (*pos != '\0' && *pos != '\n' && *pos != '\r');
      pos++;
      continue;
    }

    // TODO: use std::variant, exceptions bad
    if (!std::isspace(*pos))
      return std::format("Could not tokenize '{}'", *pos);
  }

  return result;
}

Token::Token(std::string str) {
  this->data = std::nullopt;

  if (str == "def")
    this->kind = TokenKind::Def;
  else if (str == "extern")
    this->kind = TokenKind::Extern;
  else if (str == "if")
    this->kind = TokenKind::If;
  else if (str == "then")
    this->kind = TokenKind::Then;
  else if (str == "else")
    this->kind = TokenKind::Else;
  else {
    this->kind = TokenKind::Identifier;
    this->data = OptionalTokenData(str);
  }
}

std::optional<Token> Token::from_symbol(char symbol) {
  switch (symbol) {
  case '(':
    return Token(TokenKind::ParenOpen);
  case ')':
    return Token(TokenKind::ParenClose);
  case '<':
    return Token(TokenKind::LessThan);
  case '+':
    return Token(TokenKind::Plus);
  case '-':
    return Token(TokenKind::Minus);
  default:
    return std::nullopt;
  }
}

#define TOKEN_FORMAT_CASE(kind)                                                \
  case TokenKind::kind:                                                        \
    result = #kind;                                                            \
    break;

std::string Token::format() {
  std::string result;
  switch (this->kind) {
    TOKEN_FORMAT_CASE(ParenOpen)
    TOKEN_FORMAT_CASE(ParenClose)
    TOKEN_FORMAT_CASE(LessThan)
    TOKEN_FORMAT_CASE(Minus)
    TOKEN_FORMAT_CASE(Plus)
    TOKEN_FORMAT_CASE(Def)
    TOKEN_FORMAT_CASE(Extern)
    TOKEN_FORMAT_CASE(If)
    TOKEN_FORMAT_CASE(Then)
    TOKEN_FORMAT_CASE(Else)
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

#undef TOKEN_FORMAT_CASE

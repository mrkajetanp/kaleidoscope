#include "lexer.hpp"
#include "easylogging++.h"
#include "llvm/Support/MemoryBuffer.h"
#include <cctype>
#include <print>

TokenizeResult tokenize(const llvm::MemoryBuffer *buffer) {
  std::deque<Token> result{};

  char *pos = (char *)buffer->getBufferStart();
  const char *end = buffer->getBufferEnd();
  while (pos < end) {
    while (std::isspace(*pos))
      ++pos;

    // Handle EOF
    if (*pos == '\0')
      break;

    VLOG(5) << "startpos " << *pos;

    // Handle symbols
    std::optional<Token> symbol_token = Token::from_symbol(*pos);
    if (symbol_token.has_value()) {
      result.push_back(symbol_token.value());
      VLOG(5) << std::format("adding {}", symbol_token.value());
      pos++;
      continue;
    }

    // Handle identifiers
    if (std::isalpha(*pos)) {
      std::string identifier;

      while (std::isalnum(*pos))
        identifier += *pos++;

      result.push_back(Token(identifier));
      VLOG(5) << "adding " << identifier;
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
      VLOG(5) << "adding " << value << " pos " << *pos;
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
  case '*':
    return Token(TokenKind::Asterisk);
  case ',':
    return Token(TokenKind::Comma);
  default:
    return std::nullopt;
  }
}

#define TOKEN_PRECEDENCE_CASE(kind, precedence)                                \
  case TokenKind::kind:                                                        \
    return precedence;

int Token::precedence() {
  switch (this->kind) {
    TOKEN_PRECEDENCE_CASE(LessThan, 10);
    TOKEN_PRECEDENCE_CASE(Plus, 20);
    TOKEN_PRECEDENCE_CASE(Minus, 20);
    TOKEN_PRECEDENCE_CASE(Asterisk, 40);
  default:
    return -1;
  }
}

#undef TOKEN_PRECEDENCE_CASE

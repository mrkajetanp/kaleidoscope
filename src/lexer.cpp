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

      // Handle keywords
      if (identifier == "def")
        result.push_back(Token{TokenKind::Def});
      else if (identifier == "extern")
        result.push_back(Token{TokenKind::Extern});
      else
        result.push_back(
            Token{TokenKind::Identifier, OptionalTokenData(identifier)});
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

#ifndef PARSER_H_
#define PARSER_H_

#include "ast.hpp"
#include "lexer.hpp"
#include <deque>

namespace parser {

std::vector<std::unique_ptr<ast::FunctionDefinition>> parse(std::deque<Token> &tokens);

}

#endif // PARSER_H_

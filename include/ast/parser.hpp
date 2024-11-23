#ifndef PARSER_H_
#define PARSER_H_

#include "ast.hpp"
#include "lexer.hpp"
#include <deque>

namespace parser {

std::unique_ptr<ast::CompilationUnit> parse(std::deque<Token> &tokens,
                                            std::string filename);

}

#endif // PARSER_H_

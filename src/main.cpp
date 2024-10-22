#include "lexer.hpp"
#include "parser.hpp"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include <deque>
#include <iostream>
#include <iterator>
#include <memory>
#include <print>
#include <variant>

std::unique_ptr<llvm::MemoryBuffer> read_file(std::string filepath) {
  using FileOrError = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>;
  FileOrError result = llvm::MemoryBuffer::getFileOrSTDIN(filepath);
  return std::move(result.get());
}

int compile(const llvm::MemoryBuffer *buf) {
  // Lexer
  auto lexer_result = tokenize(buf);
  if (std::holds_alternative<std::string>(lexer_result)) {
    std::print(std::cerr, "Lexer error: {}",
               std::get<std::string>(lexer_result));
    return 1;
  }
  auto tokens_vec = std::get<std::vector<Token>>(lexer_result);
  std::println("tokens: {}", tokens_vec.size());

  std::deque<Token> tokens;
  std::move(tokens_vec.begin(), tokens_vec.end(), std::back_inserter(tokens));

  for (Token &token : tokens) {
    std::print("{} ", token.format());
  }
  std::println();

  // Parser
  auto ast = parser::parse(tokens);
  for (auto &fn : ast)
    std::println("got function");

  return 0;
}

int main() {
  std::string path = "samples/basic.k";
  std::println("filepath is {}", path);
  auto buffer = read_file(path);
  llvm::SourceMgr source_manager;
  auto id = source_manager.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
  const llvm::MemoryBuffer *buf = source_manager.getMemoryBuffer(id);
  return compile(buf);
}

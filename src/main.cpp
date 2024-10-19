#include "lexer.hpp"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include <iostream>
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
  auto tokens = std::get<std::vector<Token>>(lexer_result);
  std::println("tokens: {}", tokens.size());

  for (Token &token : tokens) {
    std::print("{} ", token.format());
  }
  std::println();

  // Parser
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

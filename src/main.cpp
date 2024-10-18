#include "lexer.hpp"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include <memory>
#include <print>

std::unique_ptr<llvm::MemoryBuffer> read_file(std::string filepath) {
  using FileOrError = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>;
  FileOrError result = llvm::MemoryBuffer::getFileOrSTDIN(filepath);
  return std::move(result.get());
}

int main() {
  std::string path = "samples/basic.k";
  std::println("filepath is {}", path);
  auto buffer = read_file(path);
  llvm::SourceMgr source_manager;
  auto id = source_manager.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
  const llvm::MemoryBuffer *buf = source_manager.getMemoryBuffer(id);

  std::vector<Token> tokens = tokenize(buf);
  std::println("tokens: {}", tokens.size());

  for (Token &token : tokens) {
    std::print("{} ", token.format());
  }
  std::println();

  return 0;
}

#include "ast/parser.hpp"
#include "ast/printer.hpp"
#include "codegen.hpp"
#include "easylogging++.h"
#include "lexer.hpp"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include <deque>
#include <memory>
#include <print>
#include <variant>

INITIALIZE_EASYLOGGINGPP

std::unique_ptr<llvm::MemoryBuffer> read_file(std::string filepath) {
  using FileOrError = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>;
  FileOrError result = llvm::MemoryBuffer::getFileOrSTDIN(filepath);
  return std::move(result.get());
}

int compile(const llvm::MemoryBuffer *buf, std::string filename) {
  // Lexer
  VLOG(1) << "*** Source ***";
  VLOG(1) << '\n' << buf->getBuffer().str();
  auto lexer_result = tokenize(buf);
  if (std::holds_alternative<std::string>(lexer_result)) {
    LOG(ERROR) << "Lexer error: " << std::get<std::string>(lexer_result);
    return 1;
  }
  auto tokens = std::get<std::deque<Token>>(lexer_result);
  VLOG(1) << "*** Tokens ***";

  if (VLOG_IS_ON(1)) {
    std::stringstream tokensLog;
    for (Token &token : tokens) {
      tokensLog << std::format("{} ", token);
    }
    VLOG(1) << tokensLog.str();
  }

  // Parser
  auto ast = parser::parse(tokens, filename);
  VLOG(1) << "*** AST ***";
  VLOG(1) << '\n' << std::format("{}", *ast);

  // Codegen
  codegen::codegen(ast.get());

  return 0;
}

int main(int argc, char *argv[]) {
  START_EASYLOGGINGPP(argc, argv);

  std::string path = "samples/control_flow.k";
  VLOG(2) << "Filepath: " << path;
  auto buffer = read_file(path);
  llvm::SourceMgr source_manager;
  auto id = source_manager.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
  const llvm::MemoryBuffer *buf = source_manager.getMemoryBuffer(id);
  return compile(buf, path);
}

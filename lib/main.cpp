#include "ast/parser.hpp"
#include "ast/printer.hpp"
#include "codegen.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"
#include <deque>
#include <memory>
#include <print>
#include <sstream>
#include <variant>

// CLI parameters

llvm::cl::opt<std::string> InputFilename(llvm::cl::Positional,
                                         llvm::cl::desc("<input file>"),
                                         llvm::cl::Required);
llvm::cl::opt<std::string>
    OutputFilename("o", llvm::cl::desc("Specify output filename"),
                   llvm::cl::value_desc("filename"));

llvm::cl::opt<log::LoggingLevel>
    LoggingLevel("log", llvm::cl::desc("Choose the logging level:"),
                 llvm::cl::values(clEnumValN(log::error, "error", "Error"),
                                  clEnumValN(log::warn, "warn", "Warn"),
                                  clEnumValN(log::info, "info", "Info"),
                                  clEnumValN(log::debug, "debug", "Debug"),
                                  clEnumValN(log::trace, "trace", "Trace")));

// Driver functions
std::unique_ptr<llvm::MemoryBuffer> read_file(std::string filepath) {
  using FileOrError = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>;
  FileOrError result = llvm::MemoryBuffer::getFileOrSTDIN(filepath);
  return std::move(result.get());
}

int compile(const llvm::MemoryBuffer *buf, std::string filename) {
  // Lexer
  DEBUG("*** Source ***\n" << buf->getBuffer().str());
  auto lexer_result = tokenize(buf);
  if (std::holds_alternative<std::string>(lexer_result)) {
    ERROR("Lexer error: " << std::get<std::string>(lexer_result));
    return 1;
  }
  auto tokens = std::get<std::deque<Token>>(lexer_result);
  DEBUG("*** Tokens ***");

  if (LoggingLevel >= log::debug) {
    std::stringstream tokensLog;
    for (Token &token : tokens) {
      tokensLog << std::format("{} ", token);
    }
    DEBUG(tokensLog.str());
  }

  // Parser
  auto ast = parser::parse(tokens, filename);
  DEBUG("*** AST ***");
  DEBUG(std::format("{}", *ast));

  // Codegen
  codegen::codegen(ast.get());

  return 0;
}

int main(int argc, char *argv[]) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  auto buffer = read_file(InputFilename);
  llvm::SourceMgr source_manager;
  auto id = source_manager.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
  const llvm::MemoryBuffer *buf = source_manager.getMemoryBuffer(id);
  return compile(buf, InputFilename);
}

#ifndef LOGGER_H_
#define LOGGER_H_

#include "llvm/Support/CommandLine.h"

namespace log {

enum LoggingLevel { error, warn, info, debug, trace };

}

extern llvm::cl::opt<log::LoggingLevel> LoggingLevel;

#define ERROR(str)                                                             \
  if (LoggingLevel >= log::error) {                                            \
    llvm::outs() << "[Error] " << str << '\n';                                 \
  }

#define WARN(str)                                                              \
  if (LoggingLevel >= log::warn) {                                             \
    llvm::outs() << "[Error] " << str << '\n';                                 \
  }

#define INFO(str)                                                              \
  if (LoggingLevel >= log::info) {                                             \
    llvm::outs() << "[Info] " << str << '\n';                                  \
  }

#define DEBUG(str)                                                             \
  if (LoggingLevel >= log::debug) {                                            \
    llvm::outs() << "[Debug] " << str << '\n';                                 \
  }

#define TRACE(str)                                                             \
  if (LoggingLevel >= log::trace) {                                            \
    llvm::outs() << "[Trace] " << str << '\n';                                 \
  }

#endif // LOGGER_H_

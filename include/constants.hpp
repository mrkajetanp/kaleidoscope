#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include "logger.hpp"

extern llvm::cl::opt<std::string> InputFilename;
extern llvm::cl::opt<std::string> OutputFilename;
extern llvm::cl::opt<log::LoggingLevel> LoggingLevel;

#endif // CONSTANTS_H_

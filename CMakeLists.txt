cmake_minimum_required(VERSION 3.20.0)
project(kaleidoscope)

set(TARGET_NAME kaleidoscope)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_compile_options(-Wno-deprecated-declarations)

include_directories(include/)

# LLVM

link_directories(/usr/local/llvm/lib)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Libraries

find_package(PkgConfig REQUIRED)
pkg_check_modules(easyloggingpp easyloggingpp REQUIRED IMPORTED_TARGET)

# Executable setup

add_executable(${TARGET_NAME} lib/main.cpp lib/lexer.cpp lib/ast/parser.cpp lib/ast/printer.cpp)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)

target_link_libraries(${TARGET_NAME} PUBLIC ${llvm_libs})
target_link_libraries(${TARGET_NAME} PRIVATE PkgConfig::easyloggingpp)

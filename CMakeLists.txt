cmake_minimum_required(VERSION 3.12)
project(ASTDumpTool)


find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

add_executable(ast-dump-tool
  src/main.cpp)

target_link_libraries(ast-dump-tool PRIVATE
  clang-cpp
  LLVM-18)

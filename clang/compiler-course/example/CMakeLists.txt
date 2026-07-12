cmake_minimum_required(VERSION 3.13)

add_library(example_ClangAST MODULE
    example.cpp
)

target_include_directories(example_ClangAST PRIVATE
    /tmp/llvm-project/clang/include
    /tmp/llvm-project/llvm/include
    /usr/include
)

target_link_libraries(example_ClangAST PRIVATE
    clangAST
    clangBasic
    clangFrontend
    LLVMSupport
)

set_target_properties(example_ClangAST PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    SUFFIX ".so"
)

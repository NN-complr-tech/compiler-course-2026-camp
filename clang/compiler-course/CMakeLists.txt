cmake_minimum_required(VERSION 3.13)
project(CompilerCourse)

if (CLANG_PLUGIN_SUPPORT)
    
    add_subdirectory(zhulin_task1)    
    set(CLANG_TEST_DEPS ${CLANG_TEST_DEPS} PARENT_SCOPE)
endif()

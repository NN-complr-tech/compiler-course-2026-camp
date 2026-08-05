if(NOT WIN32 AND NOT CYGWIN)
    include_directories(${LLVM_MAIN_SRC_DIR}/lib/Target/X86)
    include_directories(${LLVM_BINARY_DIR}/lib/Target/X86)

    include_directories(${LLVM_MAIN_SRC_DIR}/lib/Target/AArch64)
    include_directories(${LLVM_BINARY_DIR}/lib/Target/AArch64)

    list_subdirs(subdirs ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirs(subdirs "COMPILER COURSE (BACKEND)")
    set(LLVM_TEST_DEPENDS ${LLVM_TEST_DEPENDS} PARENT_SCOPE)
endif()
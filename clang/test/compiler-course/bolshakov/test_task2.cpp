// RUN: %clang_cc1 -load %llvmshlibdir/bolshakov_ClangAST%pluginext -plugin null_check_plugin -fsyntax-only %s 2>&1 | FileCheck %s

// CHECK: warning: Null pointer check: use NULL as null pointer, use nullptr instead
void test_null_assignment() {
  int *p = __null;
}

// CHECK: warning: Null pointer check: use 0 as null pointer, use nullptr instead
void test_zero_assignment() {
  int *q = 0;
}

// CHECK: warning: Null pointer check: use NULL as null pointer, use nullptr instead
void func_with_pointer(int *ptr) {}
void test_null_argument() {
  func_with_pointer(__null);
}

// CHECK: warning: Null pointer check: use 0 as null pointer, use nullptr instead
void test_zero_argument() {
  func_with_pointer(0);
}

// CHECK: warning: Null pointer check: use NULL as null pointer, use nullptr instead
int* return_null() {
  return __null;
}

// CHECK: warning: Null pointer check: use 0 as null pointer, use nullptr instead
int* return_zero() {
  return 0;
}

void test_nullptr_ok() {
  int *p = nullptr;
  func_with_pointer(nullptr);
}

void test_non_pointer() {
  int x = 0;
  int y = 0;
}

// CHECK: warnings generated
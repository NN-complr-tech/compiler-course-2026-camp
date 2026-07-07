// RUN: %clang_cc1 -load %llvmshlibdir/kstrelkov_ClangAST%pluginext -plugin nodiscard-checker -fsyntax-only %s 2>&1 | FileCheck %s

// CHECK: warning: function 'should_be_nodiscard' returns a value and should be marked with {{\[\[nodiscard\]\]}} attribute
// CHECK-NEXT: {{.*}}int should_be_nodiscard() { return 42; }
// CHECK-NEXT: {{.*}}^
// CHECK-NEXT: {{.*}}{{\[\[nodiscard\]\]}}
int should_be_nodiscard() { return 42; }

// CHECK: warning: result of function 'should_be_nodiscard' is ignored
// CHECK-NEXT: {{.*}}should_be_nodiscard();
// CHECK-NEXT: {{.*}}^
void test_ignored() {
    should_be_nodiscard();
}

void returns_void() {}

[[nodiscard]] int already_has_nodiscard() { return 100; }

void test_correct_behavior() {
    returns_void();
    
    int x = should_be_nodiscard();
    
    int y = already_has_nodiscard();
}
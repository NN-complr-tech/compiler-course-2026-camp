// RUN: %clang_cc1 -load %llvmshlibdir/kstrelkov_ClangAST%pluginext -plugin nodiscard-checker -fsyntax-only %s 2>&1 | FileCheck %s

// CHECK: warning: function 'warn_standard' returns a value and should be marked with {{\[\[nodiscard\]\]}} attribute
// CHECK-NEXT: {{.*}}int warn_standard() { return 1; }
// CHECK-NEXT: {{.*}}^
// CHECK-NEXT: {{.*}}{{\[\[nodiscard\]\]}}
int warn_standard() { return 1; }

void returns_void() {}

[[nodiscard]] int already_has_attribute() { return 2; }

// CHECK: warning: function 'warn_canonical' returns a value and should be marked with {{\[\[nodiscard\]\]}} attribute
// CHECK-NEXT: {{.*}}int warn_canonical();
// CHECK-NEXT: {{.*}}^
// CHECK-NEXT: {{.*}}{{\[\[nodiscard\]\]}}
int warn_canonical();
int warn_canonical() { return 3; }

struct SpecialMethods {
  SpecialMethods() {}
  ~SpecialMethods() {}
  operator int() { return 42; }
};

struct CustomOperators {
  CustomOperators &operator<<(int) { return *this; }
  CustomOperators &operator>>(int) { return *this; }
  CustomOperators &operator=(const CustomOperators &) { return *this; }
  CustomOperators &operator+=(int) { return *this; }
  CustomOperators &operator-=(int) { return *this; }
  CustomOperators &operator*=(int) { return *this; }
  CustomOperators &operator/=(int) { return *this; }

  // CHECK: warning: function 'operator+' returns a value and should be marked with {{\[\[nodiscard\]\]}} attribute
  CustomOperators operator+(int) { return *this; }
};

struct ClassWithDestructor {
  ClassWithDestructor() {}
  ~ClassWithDestructor() {}
};

// CHECK: warning: function 'returns_class' returns a value and should be marked with {{\[\[nodiscard\]\]}} attribute
ClassWithDestructor returns_class() { return ClassWithDestructor(); }

void test_calls() {
  // CHECK: warning: result of function 'warn_standard' is ignored
  // CHECK-NEXT: {{.*}}warn_standard();
  warn_standard();

  int val = warn_standard();

  if (warn_standard()) {}

  // CHECK: warning: result of function 'warn_standard' is ignored
  // CHECK-NEXT: {{.*}}(warn_standard());
  (warn_standard());

  (void)warn_standard();

  // CHECK: warning: result of function 'returns_class' is ignored
  // CHECK-NEXT: {{.*}}returns_class();
  returns_class();
}

// Emulate a system header using preprocessor line markers.
// The flag '3' at the end marks the following lines as a system header.
#line 1 "system_header.h" 3

int system_header_function() { return 99; }

void test_system_calls() {
  system_header_function();
}
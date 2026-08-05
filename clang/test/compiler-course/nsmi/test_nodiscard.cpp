// RUN: %clang_cc1 -load %llvmshlibdir/nsmi_ClangAST%pluginext -plugin nodiscard-plugin -fsyntax-only %s 2>&1 | FileCheck %s

// CHECK-NOT: warning: function {{.*}} returning non-void should be marked with {{\[\[nodiscard\]\]}}
// CHECK-NOT: warning: result of call to non-void function {{.*}} is ignored
#include <stddef.h>

// CHECK: warning: function 'basic_warn' returning non-void should be marked with {{\[\[nodiscard\]\]}}
int basic_warn() {
    return 55;
}

// CHECK-NOT: warning: function 'void_its_ok' returning non-void should be marked with {{\[\[nodiscard\]\]}}
void void_its_ok() {
    return;
}

// CHECK: warning: function 'template_func' returning non-void should be marked with {{\[\[nodiscard\]\]}}
template<typename T>
T template_func(T val) {
    return val;
}

// CHECK-NOT: warning: function 'template_func' returning non-void should be marked with {{\[\[nodiscard\]\]}}
void test_instantiations() {
    template_func(10);
    template_func(3.14);
    template_func('a');
}

// CHECK-NOT: warning: function 'already_has_attr' returning non-void should be marked with {{\[\[nodiscard\]\]}}
[[nodiscard]] int already_has_attr() {
  return 100;
}

// CHECK-NOT: warning: function 'SpecialMethodsTest' returning non-void should be marked with {{\[\[nodiscard\]\]}}
class SpecialMethodsTest {
public:
    SpecialMethodsTest() {}
    ~SpecialMethodsTest() {}
    operator int() const {
        return 42;
    }
};

class OperatorsTest {
  int x;

public:
  // CHECK-NOT: warning: function 'operator=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator=(const OperatorsTest& Other) { x = Other.x; return *this; }

  // CHECK-NOT: warning: function 'operator+=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator+=(int n) { x += n; return *this; }

  // CHECK-NOT: warning: function 'operator-=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator-=(int n) { x -= n; return *this; }

  // CHECK-NOT: warning: function 'operator*=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator*=(int n) { x *= n; return *this; }

  // CHECK-NOT: warning: function 'operator/=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator/=(int n) { x /= n; return *this; }

  // CHECK-NOT: warning: function 'operator%=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator%=(int n) { x %= n; return *this; }

  // CHECK-NOT: warning: function 'operator&=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator&=(int n) { x &= n; return *this; }

  // CHECK-NOT: warning: function 'operator|=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator|=(int n) { x |= n; return *this; }

  // CHECK-NOT: warning: function 'operator^=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator^=(int n) { x ^= n; return *this; }

  // CHECK-NOT: warning: function 'operator<<' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator<<(int n) { x <<= n; return *this; }

  // CHECK-NOT: warning: function 'operator>>' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator>>(int n) { x >>= n; return *this; }

  // CHECK-NOT: warning: function 'operator<<=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator<<=(int n) { x <<= n; return *this; }

  // CHECK-NOT: warning: function 'operator>>=' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator>>=(int n) { x >>= n; return *this; }

  // CHECK-NOT: warning: function 'operator++' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator++() { ++x; return *this; }
  OperatorsTest operator++(int) { OperatorsTest tmp = *this; ++x; return tmp; }

  // CHECK-NOT: warning: function 'operator--' returning non-void should be marked with {{\[\[nodiscard\]\]}}
  OperatorsTest& operator--() { --x; return *this; }
  OperatorsTest operator--(int) { OperatorsTest tmp = *this; --x; return tmp; }
};

// CHECK: warning: function 'operator+' returning non-void should be marked with {{\[\[nodiscard\]\]}}
OperatorsTest operator+(const OperatorsTest& a, const OperatorsTest& b) {
    return a;
}

struct StructWithDtor {
  ~StructWithDtor() {}
};

// CHECK: warning: function 'return_struct_with_dtor' returning non-void should be marked with {{\[\[nodiscard\]\]}}
StructWithDtor return_struct_with_dtor() {
  return StructWithDtor();
}

void test_dtor() {
    // CHECK: warning: result of call to non-void function 'return_struct_with_dtor' is ignored
    return_struct_with_dtor();
}

void test_ignored_calls() {
    // CHECK: warning: result of call to non-void function 'basic_warn' is ignored
    basic_warn();

    // CHECK: warning: result of call to non-void function 'operator+' is ignored
    OperatorsTest a, b;
    a + b;

    SpecialMethodsTest obj;
    // CHECK: warning: result of call to non-void function 'operator int' is ignored
    (int)obj;
}

void test_used_calls() {
  // CHECK-NOT: warning: result of call to non-void function 'basic_warn' is ignored
  int val = basic_warn();

  // CHECK-NOT: warning: result of call to non-void function 'basic_warn' is ignored
  (void)basic_warn();

  // CHECK-NOT: warning: result of call to non-void function 'basic_warn' is ignored
  if (basic_warn() == 55) {}

  // CHECK-NOT: warning: result of call to non-void function 'void_its_ok' is ignored
  void_its_ok();

  // CHECK-NOT: warning: result of call to non-void function 'already_has_attr' is ignored
  already_has_attr(); 

  OperatorsTest op;
  OperatorsTest other;
  // CHECK-NOT: warning: result of call to non-void function 'operator=' is ignored
  op = other;

  // CHECK-NOT: warning: result of call to non-void function 'operator<<' is ignored
  op << 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator>>' is ignored
  op >> 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator+=' is ignored
  op += 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator-=' is ignored
  op -= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator*=' is ignored
  op *= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator/=' is ignored
  op /= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator%=' is ignored
  op %= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator&=' is ignored
  op &= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator|=' is ignored
  op |= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator^=' is ignored
  op ^= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator<<=' is ignored
  op <<= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator>>=' is ignored
  op >>= 5;

  // CHECK-NOT: warning: result of call to non-void function 'operator++' is ignored
  op++;
  ++op;

  // CHECK-NOT: warning: result of call to non-void function 'operator--' is ignored
  op--;
  --op;
}

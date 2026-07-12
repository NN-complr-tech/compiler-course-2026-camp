// RUN: %clang_cc1 -load %llvmshlibdir/lyzlova_ClangAST%pluginext -plugin nodiscard-checker -fsyntax-only %s 2>&1 | FileCheck %s

// CHECK: warning: function 'should_warn' returns a value and should be marked
int should_warn() {
    return 42;
}

// CHECK-NOT: function 'returns_void'
void returns_void() {
}

[[nodiscard]] int already_ok() {
    return 10;
}

struct StreamLike {
    // CHECK-NOT: function 'StreamLike::operator<<'
    StreamLike& operator<<(int) {
        return *this;
    }
};

// CHECK: warning: result of function 'should_warn' is ignored
void ignored_result() {
    should_warn();
}

void take_int(int);

void used_in_variable() {
    int value = should_warn();
    (void)value;
}

void used_in_if() {
    if (should_warn()) {
    }
}

int used_in_return() {
    return should_warn();
}

void used_as_argument() {
    take_int(should_warn());
}

void used_in_expression() {
    int x = should_warn() + 1;
    (void)x;
}

void used_in_assignment() {
    int x;
    x = should_warn();
    (void)x;
}

struct TestClass {
    // CHECK: warning: function 'TestClass::should_warn' returns a value and should be marked
    int should_warn() {
        return 42;
    }

    // CHECK-NOT: function 'TestClass::returns_void'
    void returns_void() {
    }

    // CHECK-NOT: function 'TestClass::already_ok'
    [[nodiscard]] int already_ok() {
        return 10;
    }
};

// CHECK: warning: result of function 'TestClass::should_warn' is ignored
void ignored_method_result() {
    TestClass obj;
    obj.should_warn();
}

void used_method_result() {
    TestClass obj;

    int value = obj.should_warn();
    (void)value;

    if (obj.should_warn()) {
    }

    take_int(obj.should_warn());

    int x = obj.should_warn() + 1;
    (void)x;
}

struct Number {
    // CHECK: warning: function 'Number::operator+' returns a value and should be marked
    Number operator+(const Number&) const {
        return Number{};
    }

    // CHECK: warning: function 'Number::operator==' returns a value and should be marked
    bool operator==(const Number&) const {
        return true;
    }

    // CHECK-NOT: function 'Number::operator<<'
    Number& operator<<(int) {
        return *this;
    }

    // CHECK-NOT: function 'Number::operator='
    Number& operator=(const Number&) = default;
};

// CHECK: warning: result of function 'Number::operator+' is ignored
// CHECK: warning: result of function 'Number::operator==' is ignored
void ignored_operator_result() {
    Number a, b;
    a + b;
    a == b;
}

void used_operator_result() {
    Number a, b;

    Number sum = a + b;
    (void)sum;

    bool equal = (a == b);
    (void)equal;

    a << 5;
}
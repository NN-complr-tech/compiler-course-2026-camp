// RUN: %clang_cc1 -load %llvmshlibdir/lyzlova_ClangAST%pluginext -plugin nodiscard-checker -fsyntax-only %s 2>&1 | FileCheck %s


// Declaration diagnostic

// CHECK: warning: function 'should_warn' returns a value and should be marked
int should_warn() {
    return 42;
}


// CHECK-NOT: function 'returns_void'
void returns_void() {
}


// CHECK-NOT: function 'already_ok'
[[nodiscard]] int already_ok() {
    return 10;
}


// CHECK-NOT: function 'operator<<'
struct StreamLike {
    StreamLike& operator<<(int) {
        return *this;
    }
};


// Ignored result

// CHECK: warning: result of function 'should_warn' is ignored
void ignored_result() {
    should_warn();
}

// Used result - should not warn

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

void take_int(int);


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
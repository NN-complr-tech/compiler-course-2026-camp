// RUN: %clang_cc1 -load %llvmshlibdir/zhulin_task1_ClangAST%pluginext -plugin zhulin_task1_plugin -fsyntax-only %s 2>&1 | FileCheck %s


// CHECK: warning: function 'getValue' returning non-void should be marked
int getValue() { return 42; }


// CHECK-NOT: warning: function 'alreadyMarked' returning non-void should be marked
[[nodiscard]] int alreadyMarked() { return 0; }

// CHECK-NOT: warning: function 'printMessage' returning non-void should be marked
void printMessage(const char *msg) {}


[[nodiscard]] int getSafeValue() { return 100; }


void testIgnoredResult()
{
    // CHECK: warning: ignoring return value of function 'getSafeValue'
    getSafeValue();
}


class TestClass
{
public:
    // CHECK: warning: function 'getValue' returning non-void should be marked
    int getValue() const { return m_value; }

    // CHECK-NOT: warning: function 'setValue' returning non-void should be marked
    void setValue(int v) { m_value = v; }

    // CHECK: warning: function 'funcWithNoSpace' returning non-void should be marked
    int funcWithNoSpace() { return 1; }

    // CHECK: warning: function 'funcWithSpace' returning non-void should be marked
    int funcWithSpace() { return 1; }

private:
    int m_value = 0;
};


void testFunctions()
{
    getValue();
    alreadyMarked();

    TestClass tc;
    tc.getValue();
    tc.funcWithNoSpace();
    tc.funcWithSpace(); 
} 

void testVoidCast()
{
    // CHECK: warning: ignoring return value of function 'getSafeValue'
    (void)getSafeValue();
}

// CHECK: warning: function 'main' returning non-void should be marked
int main()
{
    testIgnoredResult();
    testFunctions();
    testLambda();
    testVoidCast();
    return 0;
}


// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/lyzlova_MLIR%shlibext --pass-pipeline="builtin.module(normalize-scf-for)" %s | FileCheck %s

// Test 1: Simple case, 2 to 10 step 2 -> 0 to 4 step 1
// CHECK-LABEL: func.func @test_basic()
func.func @test_basic() {
  %c2 = arith.constant 2 : index
  %c10 = arith.constant 10 : index

  // CHECK: %[[C4:.*]] = arith.constant 4 : index
  // CHECK: scf.for %[[J:.*]] = %c0 to %[[C4]] step %c1
  // CHECK: %[[SCALED:.*]] = arith.muli %[[J]], %c2
  // CHECK: %[[I:.*]] = arith.addi %c2, %[[SCALED]]
  // CHECK: call @use(%[[I]])

  scf.for %i = %c2 to %c10 step %c2 {
    call @use(%i) : (index) -> ()
  }
  return
}

// Test 2: Non-divisible range, 2 to 9 step 2 -> 0 to 4 step 1 (ceil)
// CHECK-LABEL: func.func @test_non_divisible()
func.func @test_non_divisible() {
  %c2 = arith.constant 2 : index
  %c9 = arith.constant 9 : index

  // CHECK: %[[C4:.*]] = arith.constant 4 : index
  // CHECK: scf.for %[[J:.*]] = %c0 to %[[C4]] step %c1
  // CHECK: %[[SCALED:.*]] = arith.muli %[[J]], %c2
  // CHECK: %[[I:.*]] = arith.addi %c2, %[[SCALED]]
  // CHECK: call @use(%[[I]])

  scf.for %i = %c2 to %c9 step %c2 {
    call @use(%i) : (index) -> ()
  }
  return
}

// Test 3: Already normalized (0 to 10 step 1) -> should not change
// CHECK-LABEL: func.func @test_already_normalized()
func.func @test_already_normalized() {
  %c0 = arith.constant 0 : index
  %c10 = arith.constant 10 : index
  %c1 = arith.constant 1 : index

  // CHECK: scf.for %[[I:.*]] = %c0 to %c10 step %c1
  // CHECK-NOT: arith.muli
  // CHECK-NOT: arith.addi
  // CHECK: call @use(%[[I]])

  scf.for %i = %c0 to %c10 step %c1 {
    call @use(%i) : (index) -> ()
  }
  return
}

// Test 4: Loop with iter_args (loop-carried values)
// CHECK-LABEL: func.func @test_iter_args()
func.func @test_iter_args() -> index {
  %c0 = arith.constant 0 : index
  %c2 = arith.constant 2 : index
  %c10 = arith.constant 10 : index

  // CHECK: %[[C4:.*]] = arith.constant 4 : index
  // CHECK: scf.for %[[J:.*]] = %c0 to %[[C4]] step %c1 iter_args(%[[ACC:.*]] = %c0)
  // CHECK: %[[SCALED:.*]] = arith.muli %[[J]], %c2
  // CHECK: %[[I:.*]] = arith.addi %c2, %[[SCALED]]
  // CHECK: %[[V:.*]] = arith.addi %[[ACC]], %[[I]]
  // CHECK: scf.yield %[[V]]

  %sum = scf.for %i = %c2 to %c10 step %c2 iter_args(%acc = %c0) -> (index) {
    %v = arith.addi %acc, %i : index
    scf.yield %v : index
  }

  return %sum : index
}

// Test 5: Empty loop (upper <= lower) -> should not change
// CHECK-LABEL: func.func @test_empty()
func.func @test_empty() {
  %c10 = arith.constant 10 : index
  %c5 = arith.constant 5 : index
  %c1 = arith.constant 1 : index

  // CHECK: scf.for %[[I:.*]] = %c10 to %c5 step %c1
  // CHECK-NOT: arith.muli
  // CHECK-NOT: arith.addi
  // CHECK: call @use(%[[I]])

  scf.for %i = %c10 to %c5 step %c1 {
    call @use(%i) : (index) -> ()
  }
  return
}

// Test 6: lower = 0, but step != 1
// CHECK-LABEL: func.func @test_zero_lower()
func.func @test_zero_lower() {
  %c0 = arith.constant 0 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index

  // CHECK: %[[C4:.*]] = arith.constant 4 : index
  // CHECK: scf.for %[[J:.*]] = %c0 to %[[C4]] step %c1
  // CHECK: %[[SCALED:.*]] = arith.muli %[[J]], %c2
  // CHECK: %[[I:.*]] = arith.addi %c0, %[[SCALED]]
  // CHECK: call @use(%[[I]])

  scf.for %i = %c0 to %c8 step %c2 {
    call @use(%i) : (index) -> ()
  }
  return
}

// Test 7: step = 1, but lower != 0
// CHECK-LABEL: func.func @test_unit_step()
func.func @test_unit_step() {
  %c3 = arith.constant 3 : index
  %c8 = arith.constant 8 : index
  %c1 = arith.constant 1 : index

  // CHECK: %[[C5:.*]] = arith.constant 5 : index
  // CHECK: scf.for %[[J:.*]] = %c0 to %[[C5]] step %c1
  // CHECK: %[[SCALED:.*]] = arith.muli %[[J]], %c1
  // CHECK: %[[I:.*]] = arith.addi %c3, %[[SCALED]]
  // CHECK: call @use(%[[I]])

  scf.for %i = %c3 to %c8 step %c1 {
    call @use(%i) : (index) -> ()
  }
  return
}
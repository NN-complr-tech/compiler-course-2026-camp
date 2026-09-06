// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/kstrelkov_MLIR%shlibext --pass-pipeline="builtin.module(kstrelkov_MLIR)" %s | FileCheck %s

module {

  // CHECK-LABEL: func.func @test_standard()
  func.func @test_standard() {
    %c2 = arith.constant 2 : index
    %c10 = arith.constant 10 : index

    // CHECK: %[[C0:.*]] = arith.constant 0 : index
    // CHECK: %[[C4:.*]] = arith.constant 4 : index
    // CHECK: %[[C1:.*]] = arith.constant 1 : index
    // CHECK: %[[C2:.*]] = arith.constant 2 : index
    // CHECK: scf.for %[[IV:.*]] = %[[C0]] to %[[C4]] step %[[C1]] {
    // CHECK:   %[[MUL:.*]] = arith.muli %[[IV]], %[[C2]] : index
    // CHECK:   %[[ADD:.*]] = arith.addi %[[MUL]], %[[C2]] : index
    // CHECK:   "test.use"(%[[ADD]]) : (index) -> ()
    // CHECK: }
    scf.for %i = %c2 to %c10 step %c2 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_not_perfectly_divisible()
  func.func @test_not_perfectly_divisible() {
    %c2 = arith.constant 2 : index
    %c11 = arith.constant 11 : index

    // CHECK: %[[C0:.*]] = arith.constant 0 : index
    // CHECK: %[[C5:.*]] = arith.constant 5 : index
    // CHECK: %[[C1:.*]] = arith.constant 1 : index
    // CHECK: scf.for %{{.*}} = %[[C0]] to %[[C5]] step %[[C1]] {
    scf.for %i = %c2 to %c11 step %c2 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_zero_lb_custom_step()
  func.func @test_zero_lb_custom_step() {
    %c0 = arith.constant 0 : index
    %c10 = arith.constant 10 : index
    %c3 = arith.constant 3 : index

    // CHECK: %[[NEW_C0:.*]] = arith.constant 0 : index
    // CHECK: %[[C4:.*]] = arith.constant 4 : index
    // CHECK: %[[C1:.*]] = arith.constant 1 : index
    // CHECK: scf.for %{{.*}} = %[[NEW_C0]] to %[[C4]] step %[[C1]] {
    scf.for %i = %c0 to %c10 step %c3 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_custom_lb_step_one()
  func.func @test_custom_lb_step_one() {
    %c5 = arith.constant 5 : index
    %c10 = arith.constant 10 : index
    %c1 = arith.constant 1 : index

    // CHECK: %[[C0:.*]] = arith.constant 0 : index
    // CHECK: %[[C5_ITERS:.*]] = arith.constant 5 : index
    // CHECK: %[[C1_STEP:.*]] = arith.constant 1 : index
    // CHECK: scf.for %{{.*}} = %[[C0]] to %[[C5_ITERS]] step %[[C1_STEP]] {
    scf.for %i = %c5 to %c10 step %c1 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_already_normalized()
  func.func @test_already_normalized() {
    %c0 = arith.constant 0 : index
    %c10 = arith.constant 10 : index
    %c1 = arith.constant 1 : index

    // CHECK: scf.for %[[IV:.*]] = %{{.*}} to %{{.*}} step %{{.*}} {
    // CHECK-NEXT: "test.use"(%[[IV]])
    // CHECK-NOT: arith.muli
    // CHECK-NOT: arith.addi
    scf.for %i = %c0 to %c10 step %c1 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_dynamic_bounds
  // CHECK-SAME: (%[[LB:.*]]: index, %[[UB:.*]]: index, %[[STEP:.*]]: index)
  func.func @test_dynamic_bounds(%lb: index, %ub: index, %step: index) {
    // CHECK: scf.for %[[IV:.*]] = %[[LB]] to %[[UB]] step %[[STEP]] {
    scf.for %i = %lb to %ub step %step {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_negative_lb()
  func.func @test_negative_lb() {
    %cm2 = arith.constant -2 : index
    %c10 = arith.constant 10 : index
    %c2 = arith.constant 2 : index

    // CHECK: %[[CM2:.*]] = arith.constant -2 : index
    // CHECK: scf.for %{{.*}} = %[[CM2]] to
    scf.for %i = %cm2 to %c10 step %c2 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_negative_step()
  func.func @test_negative_step() {
    %c10 = arith.constant 10 : index
    %c0 = arith.constant 0 : index
    %cm2 = arith.constant -2 : index

    // CHECK: %[[CM2:.*]] = arith.constant -2 : index
    // CHECK: scf.for %{{.*}} = %{{.*}} to %{{.*}} step %[[CM2]]
    scf.for %i = %c10 to %c0 step %cm2 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }

  // CHECK-LABEL: func.func @test_ub_less_eq_lb()
  func.func @test_ub_less_eq_lb() {
    %c10 = arith.constant 10 : index
    %c2 = arith.constant 2 : index

    // CHECK: scf.for %{{.*}} = %{{.*}} to %{{.*}} step %{{.*}}
    // CHECK-NOT: arith.muli
    scf.for %i = %c10 to %c2 step %c2 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }
}
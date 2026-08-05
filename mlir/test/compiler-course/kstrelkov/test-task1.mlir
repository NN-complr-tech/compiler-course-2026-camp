// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/kstrelkov_MLIR%shlibext --pass-pipeline="builtin.module(kstrelkov_MLIR)" %s | FileCheck %s

module {
  func.func @test_normalize() {
    %c2 = arith.constant 2 : index
    %c10 = arith.constant 10 : index

    // CHECK: %[[C0:.*]] = arith.constant 0 : index
    // CHECK: %[[C4:.*]] = arith.constant 4 : index
    // CHECK: %[[C1:.*]] = arith.constant 1 : index
    // CHECK: %[[C2:.*]] = arith.constant 2 : index
    // CHECK: scf.for %[[J:.*]] = %[[C0]] to %[[C4]] step %[[C1]] {
    // CHECK:   %[[SCALED:.*]] = arith.muli %[[J]], %[[C2]] : index
    // CHECK:   %[[I:.*]] = arith.addi %[[SCALED]], %[[C2]] : index
    // CHECK:   "test.use"(%[[I]]) : (index) -> ()
    // CHECK: }
    scf.for %i = %c2 to %c10 step %c2 {
      "test.use"(%i) : (index) -> ()
    }
    return
  }
}
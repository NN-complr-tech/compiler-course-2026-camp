; l1pass_test.ll
; RUN: opt -load-pass-plugin %llvmshlibdir/lyzlova_LLVM_IR%pluginext \
; RUN: -passes=l1pass -S %s | FileCheck %s

; CHECK-LABEL: define i32 @test_smax
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp sgt i32 %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select i1 [[CMP]], i32 %a, i32 %b
; CHECK-NEXT:    ret i32 [[SEL]]
define i32 @test_smax(i32 %a, i32 %b) {
  %res = call i32 @llvm.smax.i32(i32 %a, i32 %b)
  ret i32 %res
}

; CHECK-LABEL: define i32 @test_smin
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp slt i32 %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select i1 [[CMP]], i32 %a, i32 %b
; CHECK-NEXT:    ret i32 [[SEL]]
define i32 @test_smin(i32 %a, i32 %b) {
  %res = call i32 @llvm.smin.i32(i32 %a, i32 %b)
  ret i32 %res
}

; CHECK-LABEL: define i32 @test_umax
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp ugt i32 %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select i1 [[CMP]], i32 %a, i32 %b
; CHECK-NEXT:    ret i32 [[SEL]]
define i32 @test_umax(i32 %a, i32 %b) {
  %res = call i32 @llvm.umax.i32(i32 %a, i32 %b)
  ret i32 %res
}

; CHECK-LABEL: define i32 @test_umin
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp ult i32 %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select i1 [[CMP]], i32 %a, i32 %b
; CHECK-NEXT:    ret i32 [[SEL]]
define i32 @test_umin(i32 %a, i32 %b) {
  %res = call i32 @llvm.umin.i32(i32 %a, i32 %b)
  ret i32 %res
}

; CHECK-LABEL: define <4 x i32> @test_smax_vector
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp sgt <4 x i32> %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select <4 x i1> [[CMP]], <4 x i32> %a, <4 x i32> %b
; CHECK-NEXT:    ret <4 x i32> [[SEL]]
define <4 x i32> @test_smax_vector(<4 x i32> %a, <4 x i32> %b) {
  %res = call <4 x i32> @llvm.smax.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret <4 x i32> %res
}

; CHECK-LABEL: define <4 x i32> @test_smin_vector
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp slt <4 x i32> %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select <4 x i1> [[CMP]], <4 x i32> %a, <4 x i32> %b
; CHECK-NEXT:    ret <4 x i32> [[SEL]]
define <4 x i32> @test_smin_vector(<4 x i32> %a, <4 x i32> %b) {
  %res = call <4 x i32> @llvm.smin.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret <4 x i32> %res
}

; CHECK-LABEL: define <4 x i32> @test_umax_vector
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp ugt <4 x i32> %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select <4 x i1> [[CMP]], <4 x i32> %a, <4 x i32> %b
; CHECK-NEXT:    ret <4 x i32> [[SEL]]
define <4 x i32> @test_umax_vector(<4 x i32> %a, <4 x i32> %b) {
  %res = call <4 x i32> @llvm.umax.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret <4 x i32> %res
}

; CHECK-LABEL: define <4 x i32> @test_umin_vector
; CHECK-NEXT:    [[CMP:%[0-9]+]] = icmp ult <4 x i32> %a, %b
; CHECK-NEXT:    [[SEL:%[0-9]+]] = select <4 x i1> [[CMP]], <4 x i32> %a, <4 x i32> %b
; CHECK-NEXT:    ret <4 x i32> [[SEL]]
define <4 x i32> @test_umin_vector(<4 x i32> %a, <4 x i32> %b) {
  %res = call <4 x i32> @llvm.umin.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret <4 x i32> %res
}

; CHECK-LABEL: define i32 @test_other_call
; CHECK-NEXT:    %res = call i32 @some_func(i32 %a, i32 %b)
; CHECK-NEXT:    ret i32 %res
define i32 @test_other_call(i32 %a, i32 %b) {
  %res = call i32 @some_func(i32 %a, i32 %b)
  ret i32 %res
}

declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)
declare i32 @llvm.umin.i32(i32, i32)

declare <4 x i32> @llvm.smax.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.smin.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.umax.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.umin.v4i32(<4 x i32>, <4 x i32>)

declare i32 @some_func(i32, i32)
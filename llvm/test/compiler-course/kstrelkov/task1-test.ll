; RUN: opt -load-pass-plugin %llvmshlibdir/kstrelkov_LLVM_IR%pluginext -passes=kstrelkov-mimax-pass -S %s | FileCheck %s

; ==============================================================================
; Declarations
; ==============================================================================

declare i8 @llvm.smax.i8(i8, i8)
declare i16 @llvm.smin.i16(i16, i16)
declare i32 @llvm.umax.i32(i32, i32)
declare i64 @llvm.umin.i64(i64, i64)

; Additional i32 declarations to keep original tests working
declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.umin.i32(i32, i32)

; ==============================================================================
; Tests for different types and intrinsics
; ==============================================================================

define i8 @test_smax_i8(i8 %a, i8 %b) {
; CHECK-LABEL: @test_smax_i8(
; CHECK-NOT: call i8 @llvm.smax.i8
; CHECK: %{{.*}} = icmp sgt i8 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i8 %a, i8 %b
  %res = call i8 @llvm.smax.i8(i8 %a, i8 %b)
  ret i8 %res
}

define i16 @test_smin_i16(i16 %a, i16 %b) {
; CHECK-LABEL: @test_smin_i16(
; CHECK-NOT: call i16 @llvm.smin.i16
; CHECK: %{{.*}} = icmp slt i16 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i16 %a, i16 %b
  %res = call i16 @llvm.smin.i16(i16 %a, i16 %b)
  ret i16 %res
}

define i32 @test_umax_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_umax_i32(
; CHECK-NOT: call i32 @llvm.umax.i32
; CHECK: %{{.*}} = icmp ugt i32 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i32 %a, i32 %b
  %res = call i32 @llvm.umax.i32(i32 %a, i32 %b)
  ret i32 %res
}

define i64 @test_umin_i64(i64 %a, i64 %b) {
; CHECK-LABEL: @test_umin_i64(
; CHECK-NOT: call i64 @llvm.umin.i64
; CHECK: %{{.*}} = icmp ult i64 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i64 %a, i64 %b
  %res = call i64 @llvm.umin.i64(i64 %a, i64 %b)
  ret i64 %res
}

; ==============================================================================
; Tests for remaining i32 intrinsics
; ==============================================================================

define i32 @test_smax_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_smax_i32(
; CHECK-NOT: call i32 @llvm.smax.i32
; CHECK: %{{.*}} = icmp sgt i32 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i32 %a, i32 %b
  %res = call i32 @llvm.smax.i32(i32 %a, i32 %b)
  ret i32 %res
}

define i32 @test_smin_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_smin_i32(
; CHECK-NOT: call i32 @llvm.smin.i32
; CHECK: %{{.*}} = icmp slt i32 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i32 %a, i32 %b
  %res = call i32 @llvm.smin.i32(i32 %a, i32 %b)
  ret i32 %res
}

define i32 @test_umin_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_umin_i32(
; CHECK-NOT: call i32 @llvm.umin.i32
; CHECK: %{{.*}} = icmp ult i32 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i32 %a, i32 %b
  %res = call i32 @llvm.umin.i32(i32 %a, i32 %b)
  ret i32 %res
}
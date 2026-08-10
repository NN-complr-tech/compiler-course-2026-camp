; RUN: opt -load-pass-plugin %llvmshlibdir/zhulin_task2_LLVM_IR%pluginext -passes=scalarize -S %s | FileCheck %s

; CHECK: %0 = extractelement <4 x i32> %a, i32 0
; CHECK: %1 = extractelement <4 x i32> %b, i32 0
; CHECK: %2 = add nsw i32 %0, %1
; CHECK: %3 = insertelement <4 x i32> undef, i32 %2, i32 0
; CHECK: %4 = extractelement <4 x i32> %a, i32 1
; CHECK: %5 = extractelement <4 x i32> %b, i32 1
; CHECK: %6 = add nsw i32 %4, %5
; CHECK: %7 = insertelement <4 x i32> %3, i32 %6, i32 1
; CHECK: %8 = extractelement <4 x i32> %a, i32 2
; CHECK: %9 = extractelement <4 x i32> %b, i32 2
; CHECK: %10 = add nsw i32 %8, %9
; CHECK: %11 = insertelement <4 x i32> %7, i32 %10, i32 2
; CHECK: %12 = extractelement <4 x i32> %a, i32 3
; CHECK: %13 = extractelement <4 x i32> %b, i32 3
; CHECK: %14 = add nsw i32 %12, %13
; CHECK: %15 = insertelement <4 x i32> %11, i32 %14, i32 3
; CHECK: ret <4 x i32> %15

define <4 x i32> @test_add(<4 x i32> %a, <4 x i32> %b) {
  %res = add nsw <4 x i32> %a, %b
  ret <4 x i32> %res
}

; CHECK: mul nuw i32 {{.*}}, {{.*}}
; CHECK: mul nuw i32 {{.*}}, {{.*}}
; CHECK: mul nuw i32 {{.*}}, {{.*}}
; CHECK: mul nuw i32 {{.*}}, {{.*}}
; CHECK: ret <4 x i32>

define <4 x i32> @test_mul(<4 x i32> %a, <4 x i32> %b) {
  %res = mul nuw <4 x i32> %a, %b
  ret <4 x i32> %res
}

; CHECK: extractelement <2 x i32> %a, i32 0
; CHECK: extractelement <2 x i32> %b, i32 0
; CHECK: add i32
; CHECK: insertelement <2 x i32> undef, i32 {{.*}}, i32 0
; CHECK: extractelement <2 x i32> %a, i32 1
; CHECK: extractelement <2 x i32> %b, i32 1
; CHECK: add i32
; CHECK: insertelement <2 x i32> {{.*}}, i32 {{.*}}, i32 1
; CHECK: ret <2 x i32>

define <2 x i32> @test_v2i32(<2 x i32> %a, <2 x i32> %b) {
  %res = add <2 x i32> %a, %b
  ret <2 x i32> %res
}

; CHECK: add <8 x i32> %a, %b
; CHECK-NOT: extractelement <8 x i32>
; CHECK-NOT: insertelement <8 x i32>
; CHECK: ret <8 x i32>

define <8 x i32> @test_no_change(<8 x i32> %a, <8 x i32> %b) {
  %res = add <8 x i32> %a, %b
  ret <8 x i32> %res
}

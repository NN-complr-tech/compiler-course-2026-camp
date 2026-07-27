; RUN: opt -load-pass-plugin %llvmshlibdir/kstrelkov_LLVM_IR%pluginext -passes=kstrelkov-mimax-pass -S %s | FileCheck %s

declare i32 @llvm.smax.i32(i32, i32)

define i32 @test_smax(i32 %a, i32 %b) {
; CHECK-LABEL: @test_smax(
; CHECK-NOT: call i32 @llvm.smax.i32
; CHECK: %{{.*}} = icmp sgt i32 %a, %b
; CHECK-NEXT: %{{.*}} = select i1 %{{.*}}, i32 %a, i32 %b

  %res = call i32 @llvm.smax.i32(i32 %a, i32 %b)
  ret i32 %res
}

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

namespace {

struct L1Pass : llvm::PassInfoMixin<L1Pass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &) {
    bool Changed = false;
    // In this pass we should go thru all:
    // base blocks
    // instructions in those
    for (auto &BB : F) {
      for (auto Inst = BB.begin(), End = BB.end(); Inst != End;) {
        auto *Call = llvm::dyn_cast<llvm::CallInst>(&*Inst);
        // we should check that this inst is CALL not something else like add
        if (!Call) { ++Inst; continue; }
        auto *Callee = Call->getCalledFunction();
        // then check if it is pointer. it will return nullptr if it is - then we dont need it, not with intrinsics!
        if (!Callee) { ++Inst; continue; }

        // getIntrinsicID() returns the ID of a known intrinsic
        // if it matches one of the four, we save the required comparison predicate
        // Otherwise, we move on to the next instruction!
        llvm::CmpInst::Predicate Pred;
        switch (Callee->getIntrinsicID()) {
          case llvm::Intrinsic::smax: Pred = llvm::CmpInst::ICMP_SGT; break;
          case llvm::Intrinsic::smin: Pred = llvm::CmpInst::ICMP_SLT; break;
          case llvm::Intrinsic::umax: Pred = llvm::CmpInst::ICMP_UGT; break;
          case llvm::Intrinsic::umin: Pred = llvm::CmpInst::ICMP_ULT; break;
          default: ++Inst; continue;
        }
        // We raise the Changed flag cause we're definitely making a replacement
        //then we take the call arguments (there are always two)
        // IRBuilder(Call) creates a builder that will insert new instructions before Call
        Changed = true;   
        auto *LHS = Call->getArgOperand(0);
        auto *RHS = Call->getArgOperand(1);
        llvm::IRBuilder<> Builder(Call);
        auto *Cmp = Builder.CreateICmp(Pred, LHS, RHS);
        auto *Sel = Builder.CreateSelect(Cmp, LHS, RHS);
        //All instructions that used the result of Call now use Sel
        // eraseFromParent() removes Call from the base block and returns an iterator to the next instruction
        //  We assign it to Inst so the loop continues correctly.
        Call->replaceAllUsesWith(Sel);
        Inst = Call->eraseFromParent();
      }
    }
    

    // If the pass didn't change anything, we return all(). the pass manager doesn't reset the caches of other analyses, it saves time
    //If there were changes, we return none(), because we don't know which analyses remained valid after our replacement.
    return Changed ? llvm::PreservedAnalyses::none()
                   : llvm::PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION,
      "L1Pass",
      LLVM_VERSION_STRING,
      [](llvm::PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](llvm::StringRef Name,
               llvm::FunctionPassManager &FPM,
               llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
              if (Name == "l1pass") {
                FPM.addPass(L1Pass{});
                return true;
              }
              return false;
            });
      }};
}
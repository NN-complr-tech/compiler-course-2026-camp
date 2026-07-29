#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

namespace {
struct MinMaxDecompositionPass : llvm::PassInfoMixin<MinMaxDecompositionPass> {
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &) {
    bool Changed = false;

    std::vector<llvm::CallInst *> Worklist;

    for (llvm::BasicBlock &BB : F) {
      for (llvm::Instruction &I : BB) {
        if (auto *Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
          switch (Call->getIntrinsicID()) {
          case llvm::Intrinsic::smax:
          case llvm::Intrinsic::smin:
          case llvm::Intrinsic::umax:
          case llvm::Intrinsic::umin:
            Worklist.push_back(Call);
            break;
          default:
            break;
          }
        }
      }
    }

    for (llvm::CallInst *Call : Worklist) {
      llvm::IRBuilder<> Builder(Call);

      llvm::Value *LHS = Call->getArgOperand(0);
      llvm::Value *RHS = Call->getArgOperand(1);

      llvm::Value *Cmp = nullptr;
      llvm::Value *Sel = nullptr;

      switch (Call->getIntrinsicID()) {
      case llvm::Intrinsic::smax:
        Cmp = Builder.CreateICmpSGT(LHS, RHS);
        Sel = Builder.CreateSelect(Cmp, LHS, RHS);
        break;

      case llvm::Intrinsic::smin:
        Cmp = Builder.CreateICmpSLT(LHS, RHS);
        Sel = Builder.CreateSelect(Cmp, LHS, RHS);
        break;

      case llvm::Intrinsic::umax:
        Cmp = Builder.CreateICmpUGT(LHS, RHS);
        Sel = Builder.CreateSelect(Cmp, LHS, RHS);
        break;

      case llvm::Intrinsic::umin:
        Cmp = Builder.CreateICmpULT(LHS, RHS);
        Sel = Builder.CreateSelect(Cmp, LHS, RHS);
        break;

      default:
        llvm_unreachable("Unexpected intrinsic in worklist");
      }

      if (Sel) {
        Call->replaceAllUsesWith(Sel);
        Call->eraseFromParent();
        Changed = true;
      }
    }

    if (Changed) {
      return llvm::PreservedAnalyses::none();
    }
    return llvm::PreservedAnalyses::all();
  }

  //static bool isRequired() { return true; }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MinMaxDecompositionPass", "0.1",
          [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) -> bool {
                  if (Name == "kstrelkov-mimax-pass") {
                    FPM.addPass(MinMaxDecompositionPass{});
                    return true;
                  }
                  return false;
                });
          }};
}
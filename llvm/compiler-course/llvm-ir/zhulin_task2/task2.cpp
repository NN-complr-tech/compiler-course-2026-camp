#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <string> 

namespace {

struct ScalarizePass : llvm::PassInfoMixin<ScalarizePass> {
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &) {
    std::vector<llvm::BinaryOperator *> Targets;
    bool changed = false; 
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *BO = llvm::dyn_cast<llvm::BinaryOperator>(&I)) {
          if (auto *VT = llvm::dyn_cast<llvm::FixedVectorType>(BO->getType())) {
            if (VT->getNumElements() <= 4) {
              Targets.push_back(BO);
            }
          }
        }
      }
    }

    for (llvm::BinaryOperator *BO : Targets) {
      auto *VT = llvm::cast<llvm::FixedVectorType>(BO->getType());
      int NumElements = VT->getNumElements();
      
      llvm::Value *LV = BO->getOperand(0);
      llvm::Value *RV = BO->getOperand(1);

      llvm::IRBuilder<> Builder(BO);
      

      llvm::Value *undefVec = llvm::UndefValue::get(VT);
      llvm::Type *Int32Ty = llvm::Type::getInt32Ty(F.getContext());

      for (int i = 0; i < NumElements; ++i) {
        llvm::Value *Idx = llvm::ConstantInt::get(Int32Ty, i);
        
        llvm::Value *ScalarLV = Builder.CreateExtractElement(LV, Idx);
        llvm::Value *ScalarRV = Builder.CreateExtractElement(RV, Idx);
        
        llvm::Value *ScalarRes = Builder.CreateBinOp(
            BO->getOpcode(), 
            ScalarLV, 
            ScalarRV, 
            BO->getName() + ".s" + std::to_string(i)
        );

        if (auto *NewBO = llvm::dyn_cast<llvm::BinaryOperator>(ScalarRes)) {
          NewBO->copyIRFlags(BO);
        }
        
        undefVec = Builder.CreateInsertElement(undefVec, ScalarRes, Idx);
      }

      BO->replaceAllUsesWith(undefVec);
      
      BO->eraseFromParent();
      changed = true;
      }
      if (changed){ 
            return llvm::PreservedAnalyses::none();

      }
    return llvm::PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ScalarizePass", "0.1",
          [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) -> bool {
                  if (Name == "scalarize") { 
                    FPM.addPass(ScalarizePass{});
                    return true;
                  }
                  return false;
                });
          }};
}

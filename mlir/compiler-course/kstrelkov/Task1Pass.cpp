#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Tools/Plugins/PassPlugin.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;

namespace {

struct NormalizeForLoopPattern : public OpRewritePattern<scf::ForOp> {
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    APInt lbInt, ubInt, stepInt;
    if (!matchPattern(forOp.getLowerBound(), m_ConstantInt(&lbInt)) ||
        !matchPattern(forOp.getUpperBound(), m_ConstantInt(&ubInt)) ||
        !matchPattern(forOp.getStep(), m_ConstantInt(&stepInt)))
      return failure();

    int64_t lbVal = lbInt.getSExtValue();
    int64_t ubVal = ubInt.getSExtValue();
    int64_t stepVal = stepInt.getSExtValue();

    if (lbVal < 0 || stepVal <= 0 || ubVal <= lbVal)
      return failure();

    if (lbVal == 0 && stepVal == 1)
      return failure();

    Location loc = forOp.getLoc();
    rewriter.setInsertionPoint(forOp);
    int64_t numIters = (ubVal - lbVal + stepVal - 1) / stepVal;

    Value newLb = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value newUb = rewriter.create<arith::ConstantIndexOp>(loc, numIters);
    Value newStep = rewriter.create<arith::ConstantIndexOp>(loc, 1);

    Value oldLb = rewriter.create<arith::ConstantIndexOp>(loc, lbVal);
    Value oldStep = rewriter.create<arith::ConstantIndexOp>(loc, stepVal);

    rewriter.modifyOpInPlace(forOp, [&]() {
      forOp.setLowerBound(newLb);
      forOp.setUpperBound(newUb);
      forOp.setStep(newStep);
    });

    rewriter.setInsertionPointToStart(forOp.getBody());
    Value newIv = forOp.getInductionVar();

    Value scaled = rewriter.create<arith::MulIOp>(loc, newIv, oldStep);
    Value oldIvReplacement = rewriter.create<arith::AddIOp>(loc, oldLb, scaled);

    for (OpOperand &use : llvm::make_early_inc_range(newIv.getUses())) {
      Operation *user = use.getOwner();
      if (user == scaled.getDefiningOp())
        continue;
      rewriter.modifyOpInPlace(user, [&]() { use.set(oldIvReplacement); });
    }

    return success();
  }
};

class NormalizeForLoopPass
    : public PassWrapper<NormalizeForLoopPass, OperationPass<ModuleOp>> {
public:
  StringRef getArgument() const final { return "kstrelkov_MLIR"; }
  StringRef getDescription() const final {
    return "Normalize scf.for loops to lower bound zero and step one";
  }

  void runOnOperation() override {
    Operation *op = getOperation();
    MLIRContext *context = &getContext();

    RewritePatternSet patterns(context);
    patterns.add<NormalizeForLoopPattern>(context);

    if (failed(applyPatternsGreedily(op, std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

MLIR_DECLARE_EXPLICIT_TYPE_ID(NormalizeForLoopPass) // NOLINT
MLIR_DEFINE_EXPLICIT_TYPE_ID(NormalizeForLoopPass)  // NOLINT

static mlir::PassPluginLibraryInfo getNormalizeForLoopPassPluginInfo() {
  return {MLIR_PLUGIN_API_VERSION, "NormalizeForLoopPass", "1.0",
          []() { mlir::PassRegistration<NormalizeForLoopPass>(); }};
}

extern "C" LLVM_ATTRIBUTE_WEAK mlir::PassPluginLibraryInfo
mlirGetPassPluginInfo() {
  return getNormalizeForLoopPassPluginInfo();
}
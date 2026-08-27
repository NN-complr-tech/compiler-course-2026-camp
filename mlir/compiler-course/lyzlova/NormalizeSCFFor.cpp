#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Tools/Plugins/PassPlugin.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace {

class NormalizeForLoopPattern : public OpRewritePattern<scf::ForOp> {
public:
  using OpRewritePattern<scf::ForOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(scf::ForOp forOp,
                                PatternRewriter &rewriter) const override {
    // first of all work with values of for: get bounds and step then check if they are correct - const
    auto lowerConst = forOp.getLowerBound().getDefiningOp<arith::ConstantIndexOp>();
    auto upperConst = forOp.getUpperBound().getDefiningOp<arith::ConstantIndexOp>();
    auto stepConst = forOp.getStep().getDefiningOp<arith::ConstantIndexOp>();

    if (!lowerConst || !upperConst || !stepConst)
      return failure();

      // then get some digits, not mlir values
    int64_t lower = lowerConst.value();
    int64_t upper = upperConst.value();
    int64_t step = stepConst.value();

    if (lower < 0 || upper < 0 || step <= 0)
      return failure();

    // this case means that for already is normalized
    if (lower == 0 && step == 1)
      return failure();

    if (upper <= lower)
      return failure();

    // then work with old for - its a block, we take what we need like body, arguments (0 - induction value, next ones - iter args)
    // and terminator - returns values from a region and ends its execution (yield)
    Block *oldBody = forOp.getBody();
    auto oldArgs = oldBody->getArguments();
    Operation *oldYield = oldBody->getTerminator();

    // checks of cycle structure - args and terminator
    if (oldArgs.size() != 1 + forOp.getNumRegionIterArgs())
      return failure();
    if (!oldYield || oldYield->getNumOperands() != forOp.getNumRegionIterArgs())
      return failure();

    // calculate iterations with formula N = ceil((upper - lower) / step)
    int64_t diff = upper - lower;
    int64_t numIter = diff / step + (diff % step != 0);

    // storing location for creating a new cycle
    Location loc = forOp.getLoc();

    // create new consts 0, N,1
    Value newLower = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value newUpper = rewriter.create<arith::ConstantIndexOp>(loc, numIter);
    Value newStep = rewriter.create<arith::ConstantIndexOp>(loc, 1);

    // create new for with new iter_args
    scf::ForOp newForOp = rewriter.create<scf::ForOp>(
        loc, newLower, newUpper, newStep, forOp.getInits());

    // we will use mapping to copy the whole body except old %i
    IRMapping mapping;
    //i = lower + j * step
    Block *newBody = newForOp.getBody();
    // Now all created operations will be added to the beginning
    rewriter.setInsertionPointToStart(newBody);
    Value j = newForOp.getInductionVar();
    Value stepVal = stepConst.getResult();
    Value lowerVal = lowerConst.getResult();

    Value scaled = rewriter.create<arith::MulIOp>(loc, j, stepVal);
    Value originalIndVar = rewriter.create<arith::AddIOp>(loc, lowerVal, scaled);

    // when cloning an old body, every time when encounter an old induction variable use originalIndVar
    mapping.map(forOp.getInductionVar(), originalIndVar);

    // mapping iter_args
    auto newArgs = newBody->getArguments();

    for (unsigned i = 1; i < oldArgs.size(); ++i) {
      // begin with 1 because oldArgs[0] its an indValue
      mapping.map(oldArgs[i], newArgs[i]);
    }

    // clone all ops except terminatr
    auto &oldOps = oldBody->getOperations();

    Operation *newYield = newBody->getTerminator();
    rewriter.setInsertionPoint(newYield);

    for (auto it = oldOps.begin(); it != oldOps.end(); ++it) {
      Operation &op = *it;
      if (op.hasTrait<OpTrait::IsTerminator>())
        continue;

      Operation *clonedOp = rewriter.clone(op, mapping);
      rewriter.insert(clonedOp);
    }
    // now terinator
    // if the number of elements increases more than 4, SmallVector can still grow like a normal moving array
    SmallVector<Value, 4> newYieldOperands;
    for (Value operand : oldYield->getOperands()) {
      Value mapped = mapping.lookupOrDefault(operand);
      newYieldOperands.push_back(mapped);
    }

    rewriter.setInsertionPoint(newYield);
    rewriter.replaceOpWithNewOp<scf::YieldOp>(newYield, newYieldOperands);

    // finally replace
    rewriter.replaceOp(forOp, newForOp->getResults());

    return success();
  }
};
/* 
In general, in my pass, I follow this path:
1.The old loop remains
2.New constants are created it
3.A completely new scf.for is created
4.Inside the new scf.for, an expression is created i = lower + j * step
5.The old body is copied into the new body
6.During the copying, the old %i is replaced with the calculated i (mapping)
7.The old scf.yield is replaced with the correct new scf.yield
8.All uses of the old loop's results are replaced with the results of the new one
9.The old loop is deleted
*/

class NormalizeSCFForPass
    : public PassWrapper<NormalizeSCFForPass, OperationPass<ModuleOp>> {
public:
  StringRef getArgument() const final { return "normalize-scf-for"; }

  StringRef getDescription() const final {
    return "Normalize scf.for loops to start from 0 and step 1";
  }

  void runOnOperation() override {
    ModuleOp moduleOp = getOperation();

    RewritePatternSet patterns(&getContext());
    patterns.add<NormalizeForLoopPattern>(&getContext());

    if (failed(applyPatternsGreedily(moduleOp, std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

MLIR_DECLARE_EXPLICIT_TYPE_ID(NormalizeSCFForPass)
MLIR_DEFINE_EXPLICIT_TYPE_ID(NormalizeSCFForPass)

mlir::PassPluginLibraryInfo getNormalizeSCFForPassPluginInfo() {
  return {MLIR_PLUGIN_API_VERSION, "NormalizeSCFFor", "1.0",
          []() { mlir::PassRegistration<NormalizeSCFForPass>(); }};
}

extern "C" LLVM_ATTRIBUTE_WEAK mlir::PassPluginLibraryInfo
mlirGetPassPluginInfo() {
  return getNormalizeSCFForPassPluginInfo();
}
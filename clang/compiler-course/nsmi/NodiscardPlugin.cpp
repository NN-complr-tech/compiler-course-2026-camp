#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

using namespace clang;

namespace {
class NodiscardVisitor : public RecursiveASTVisitor<NodiscardVisitor> {
  ASTContext &Context;
  DiagnosticsEngine &Diags;

  unsigned WarnNodiscard;
  unsigned WarnIgnoredResult;

public:
  explicit NodiscardVisitor(ASTContext &Context, DiagnosticsEngine &Diags)
  : Context(Context), Diags(Diags) {
    WarnNodiscard = Diags.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "function %0 returning non-void should be marked with [[nodiscard]]");

    WarnIgnoredResult = Diags.getCustomDiagID(
      DiagnosticsEngine::Warning,
      "result of call to non-void function %0 is ignored");
  }

  bool SkipFunctionDecl(FunctionDecl *FD) {
    if (!FD) {
      return true;
    }

    if (isa<CXXConstructorDecl>(FD) || isa<CXXDestructorDecl>(FD)) {
      return true;
    }

    if (FD->hasAttr<WarnUnusedResultAttr>()) {
      return true;
    }

    if (FD->getReturnType()->isVoidType()) {
      return true;
    }

    if (FD->isOverloadedOperator()) {
      switch (FD->getOverloadedOperator()) {
        case OO_Equal:
        case OO_LessLess:
        case OO_GreaterGreater:
        case OO_PlusEqual:
        case OO_MinusEqual:
        case OO_StarEqual:
        case OO_SlashEqual:
        case OO_PercentEqual:
        case OO_AmpEqual:
        case OO_PipeEqual:
        case OO_CaretEqual:
        case OO_LessLessEqual:
        case OO_GreaterGreaterEqual:
        case OO_PlusPlus:
        case OO_MinusMinus:
          return true;

        default:
          break;
      }
    }

    return false;
  }

  bool VisitFunctionDecl(FunctionDecl *FD) {
    if (!FD) return true;

    if (Context.getSourceManager().isInSystemHeader(FD->getLocation())) {
      return true;
    }

    if (FD->isTemplateInstantiation()) {
      return true;
    }

    if (isa<CXXConversionDecl>(FD)) return true;
    
    if (SkipFunctionDecl(FD)) return true;

    Diags.Report(FD->getLocation(), WarnNodiscard) << FD;
    return true;
  }

  bool VisitCallExpr(CallExpr *CE) {
    if (!CE) return true;

    if (Context.getSourceManager().isInSystemHeader(CE->getExprLoc())) {
      return true;
    }

    FunctionDecl *FD = CE->getDirectCallee();
  
    if (SkipFunctionDecl(FD)) return true;

    DynTypedNode CurrNode = DynTypedNode::create(*CE);
    while (true) {
      DynTypedNodeList Parents = Context.getParents(CurrNode);
      if (Parents.empty()) break;

      const DynTypedNode &Parent = Parents[0];
    
      if (Parent.get<CompoundStmt>()) {
        Diags.Report(CE->getExprLoc(), WarnIgnoredResult) << FD;
        return true;
      }

      if (const Expr *E = Parent.get<Expr>()) {
        if (E->getType()->isVoidType()) {
          return true;
        }

        CurrNode = Parent;
      } else {
        break;
      }
    }

    return true;
  }
};

class NodiscardConsumer : public ASTConsumer {
  CompilerInstance &Instance;

public:
  explicit NodiscardConsumer(CompilerInstance &Instance)
      : Instance(Instance) {}

  void HandleTranslationUnit(ASTContext &Context) override {
    NodiscardVisitor Visitor(Context, Instance.getDiagnostics());
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
};


class NodiscardPluginAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
    llvm::StringRef) override {
    return std::make_unique<NodiscardConsumer>(CI);
  }

  bool ParseArgs(const CompilerInstance &CI,
     const std::vector<std::string> &args) override {
    return true;
  }
};

} // namespace

static FrontendPluginRegistry::Add<NodiscardPluginAction>
    X("nodiscard-plugin", "Suggest [[nodiscard]] and warn on ignored results");


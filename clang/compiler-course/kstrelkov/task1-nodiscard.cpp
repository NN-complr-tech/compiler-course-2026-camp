#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

namespace {

class NoDiscardVisitor final
    : public clang::RecursiveASTVisitor<NoDiscardVisitor> {
public:
  explicit NoDiscardVisitor(clang::ASTContext &ASTCtx)
      : Context(ASTCtx), Diags(ASTCtx.getDiagnostics()) {

    WarnNoDiscardID = Diags.getCustomDiagID(
        clang::DiagnosticsEngine::Warning,
        "function %0 returns a value and should be marked with "
        "[[nodiscard]] attribute");

    WarnIgnoredResultID = Diags.getCustomDiagID(
        clang::DiagnosticsEngine::Warning, "result of function %0 is ignored");
  }

  bool VisitFunctionDecl(clang::FunctionDecl *FD) {
    clang::SourceLocation Loc = FD->getLocation();

    if (Context.getSourceManager().isInSystemHeader(Loc))
      return true;

    if (FD != FD->getCanonicalDecl())
      return true;

    if (FD->hasAttr<clang::WarnUnusedResultAttr>())
      return true;

    if (!shouldBeNoDiscard(FD))
      return true;

    clang::SourceLocation BeginLoc = FD->getBeginLoc();
    clang::FixItHint FixIt =
        clang::FixItHint::CreateInsertion(BeginLoc, "[[nodiscard]] ");

    Diags.Report(Loc, WarnNoDiscardID) << FD->getDeclName() << FixIt;

    return true;
  }

  bool VisitCallExpr(clang::CallExpr *CE) {
    const clang::FunctionDecl *Callee = CE->getDirectCallee();
    if (!Callee)
      return true;

    bool IsNoDiscard = Callee->hasAttr<clang::WarnUnusedResultAttr>() ||
                       shouldBeNoDiscard(Callee);
    if (!IsNoDiscard)
      return true;

    clang::SourceLocation Loc = CE->getExprLoc();

    auto Parents = Context.getParents(*CE);
    const clang::Stmt *ParentStmt = nullptr;
    if (!Parents.empty()) {
      ParentStmt = Parents.begin()->get<clang::Stmt>();
    }

    while (ParentStmt &&
           (clang::isa<clang::ExprWithCleanups>(ParentStmt) ||
            clang::isa<clang::ParenExpr>(ParentStmt) ||
            clang::isa<clang::ImplicitCastExpr>(ParentStmt) ||
            clang::isa<clang::CXXBindTemporaryExpr>(ParentStmt) ||
            clang::isa<clang::MaterializeTemporaryExpr>(ParentStmt))) {
      auto NextParents = Context.getParents(*ParentStmt);
      ParentStmt = NextParents.empty()
                       ? nullptr
                       : NextParents.begin()->get<clang::Stmt>();
    }

    if (clang::isa_and_nonnull<clang::CompoundStmt>(ParentStmt)) {
      Diags.Report(Loc, WarnIgnoredResultID) << Callee->getDeclName();
    }

    return true;
  }

private:
  clang::ASTContext &Context;
  clang::DiagnosticsEngine &Diags;
  unsigned WarnNoDiscardID;
  unsigned WarnIgnoredResultID;

  bool shouldBeNoDiscard(const clang::FunctionDecl *FD) {
    if (!FD)
      return false;

    clang::QualType RetType = FD->getReturnType();

    if (RetType->isVoidType())
      return false;

    if (clang::isa<clang::CXXConstructorDecl>(FD) ||
        clang::isa<clang::CXXDestructorDecl>(FD) ||
        clang::isa<clang::CXXConversionDecl>(FD)) {
      return false;
    }

    if (FD->isOverloadedOperator()) {
      clang::OverloadedOperatorKind OOK = FD->getOverloadedOperator();

      if (OOK == clang::OO_LessLess || OOK == clang::OO_GreaterGreater)
        return false;

      if (OOK == clang::OO_Equal || OOK == clang::OO_PlusEqual ||
          OOK == clang::OO_MinusEqual || OOK == clang::OO_StarEqual ||
          OOK == clang::OO_SlashEqual) {
        return false;
      }
    }

    return true;
  }
};

class NoDiscardConsumer final : public clang::ASTConsumer {
public:
  explicit NoDiscardConsumer(clang::ASTContext &ASTCtx) : Visitor(ASTCtx) {}

  void HandleTranslationUnit(clang::ASTContext &ASTCtx) override {
    Visitor.TraverseDecl(ASTCtx.getTranslationUnitDecl());
  }

private:
  NoDiscardVisitor Visitor;
};

class NoDiscardAction final : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override {
    return std::make_unique<NoDiscardConsumer>(CI.getASTContext());
  }

  bool ParseArgs(const clang::CompilerInstance &CI,
                 const std::vector<std::string> &Args) override {
    return true;
  }
};

} // namespace

static clang::FrontendPluginRegistry::Add<NoDiscardAction>
    X("nodiscard-checker", "Checks for missing [[nodiscard]] attributes");
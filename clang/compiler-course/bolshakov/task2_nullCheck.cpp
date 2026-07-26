#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Basic/Diagnostic.h"

namespace {
class NullCheckVisitor final : public clang::RecursiveASTVisitor<NullCheckVisitor> {
public:
  explicit NullCheckVisitor(clang::ASTContext *context) : m_context(context) {}
  bool VisitBinaryOperator(clang::BinaryOperator *op) {
    if (!op->isAssignmentOp()) return true;  
    clang::Expr *rhs = op->getRHS();
    checkForNullLiteral(rhs);  
    return true;
}
bool VisitVarDecl(clang::VarDecl *var) {
    if (!var->getType()->isPointerType())
      return true;
    if (var->hasInit()) {
      checkForNullLiteral(var->getInit());
    }
    return true;
}
bool VisitCallExpr(clang::CallExpr *call) {
    clang::FunctionDecl *func = call->getDirectCallee();
    if (!func)  
      return true;
    for (unsigned i = 0; i < call->getNumArgs(); ++i) {
      clang::Expr *arg = call->getArg(i);
      if (i < func->getNumParams()) {
        clang::QualType paramType = func->getParamDecl(i)->getType();
        if (paramType->isPointerType()) {
          checkForNullLiteral(arg);
        }
      }
    }

    return true;
}
bool VisitReturnStmt(clang::ReturnStmt *ret) {
    clang::FunctionDecl *func = 
      clang::dyn_cast<clang::FunctionDecl>(m_currentFunction);
    
    if (func && func->getReturnType()->isPointerType()) {
      clang::Expr *retVal = ret->getRetValue();
      if (retVal) {
        checkForNullLiteral(retVal);
      }
    }

    return true;
}
bool TraverseFunctionDecl(clang::FunctionDecl *func) {
    m_currentFunction = func; 
    bool result = RecursiveASTVisitor::TraverseFunctionDecl(func);
    m_currentFunction = nullptr; 
    return result;
}

private:
  clang::ASTContext *m_context;
  clang::Decl *m_currentFunction = nullptr;
  void reportWarning(clang::Expr *expr, llvm::StringRef message) {
    if (!expr->getExprLoc().isValid())
      return;
    clang::DiagnosticsEngine &diags = m_context->getDiagnostics();
    unsigned diagID = diags.getCustomDiagID(
      clang::DiagnosticsEngine::Warning,
      "Null pointer check: %0, use nullptr instead"
    );
    diags.Report(expr->getExprLoc(), diagID) << message;
}
  void checkForNullLiteral(clang::Expr *expr) {
    expr = expr->IgnoreParenCasts();
    if (clang::IntegerLiteral *intLit = 
          clang::dyn_cast<clang::IntegerLiteral>(expr)) {
      if (intLit->getValue() == 0) {
        reportWarning(expr, "use 0 as null pointer");
        return;
      }
    }
    if (clang::GNUNullExpr *gnuNull = 
          clang::dyn_cast<clang::GNUNullExpr>(expr)) {
      reportWarning(expr, "use NULL as null pointer");
      return;
    }
}
};

class NullCheckConsumer final : public clang::ASTConsumer {
public:
  explicit NullCheckConsumer(clang::ASTContext *context) : m_visitor(context) {}

  void HandleTranslationUnit(clang::ASTContext &context) override {
    m_visitor.TraverseDecl(context.getTranslationUnitDecl());
  }

private:
  NullCheckVisitor m_visitor;
};

class NullCheckAction final : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override {
    return std::make_unique<NullCheckConsumer>(&ci.getASTContext());
  }

  bool ParseArgs(const clang::CompilerInstance &ci,
                 const std::vector<std::string> &args) override {
    return true;
  }
};
} // namespace

static clang::FrontendPluginRegistry::Add<NullCheckAction>
    X("null_check_plugin", "Checks for using 0 or NULL in pointers");

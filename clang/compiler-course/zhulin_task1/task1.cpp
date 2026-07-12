#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/AST/Attr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

class NodiscardVisitor : public RecursiveASTVisitor<NodiscardVisitor> {
public:
    explicit NodiscardVisitor(ASTContext *context) 
        : m_context(context), 
          m_diagEngine(&context->getDiagnostics()) {
        
        m_diagMissingAttr = m_diagEngine->getCustomDiagID(
            DiagnosticsEngine::Warning,
            "function returning non-void should be marked '[[nodiscard]]'"
        );
        
        m_diagIgnoredResult = m_diagEngine->getCustomDiagID(
            DiagnosticsEngine::Warning,
            "ignoring return value of function marked '[[nodiscard]]'"
        );
    }
    
    bool VisitFunctionDecl(FunctionDecl *func) {
        if (!func || func->isImplicit()) {
            return true;
        }
        
        if (isStreamOperator(func)) {
            return true;
        }
        
        if (func->getReturnType()->isVoidType()) {
            return true;
        }
        
        if (!func->hasAttr<WarnUnusedResultAttr>()) {
            DiagnosticBuilder diag = m_diagEngine->Report(
                func->getLocation(),
                m_diagMissingAttr
            );
            diag << func->getDeclName();
       
            SourceLocation startLoc = func->getBeginLoc();
            if (startLoc.isValid()) {
                diag << FixItHint::CreateInsertion(
                    startLoc,
                    "[[nodiscard]] "
                );
            }
        }
        
        return true;
    }
    
    bool VisitCallExpr(CallExpr *call) {
        if (!call) {
            return true;
        }
        
        const FunctionDecl *calledFunc = call->getDirectCallee();
        if (!calledFunc) {
            return true;
        }
        
        if (calledFunc->hasAttr<WarnUnusedResultAttr>()) {
            if (isResultIgnored(call)) {
                m_diagEngine->Report(
                    call->getBeginLoc(),
                    m_diagIgnoredResult
                ) << calledFunc->getDeclName();
            }
        }
        
        return true;
    }
    
private:
    bool isStreamOperator(FunctionDecl *func) {
        if (auto *method = dyn_cast<CXXMethodDecl>(func)) {
            if (method->isOverloadedOperator()) {
                auto opKind = method->getOverloadedOperator();
                if (opKind == OO_LessLess || opKind == OO_GreaterGreater) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool isResultIgnored(CallExpr *call) {
        auto parents = m_context->getParents(*call);
        if (parents.empty()) {
            return true;
        }
        
        const auto &parent = parents[0];
        
        if (const Stmt *parentStmt = parent.get<Stmt>()) {
            if (isa<IfStmt>(parentStmt) ||
                isa<WhileStmt>(parentStmt) ||
                isa<DoStmt>(parentStmt) ||
                isa<ForStmt>(parentStmt) ||
                isa<ReturnStmt>(parentStmt) ||
                isa<BinaryOperator>(parentStmt) ||
                isa<CallExpr>(parentStmt) ||
                isa<ConditionalOperator>(parentStmt)) {
                return false;
            }
            
            if (isa<CompoundStmt>(parentStmt) ||
                isa<ExprWithCleanups>(parentStmt)) {
                return true;
            }
        }
        
        if (const Decl *parentDecl = parent.get<Decl>()) {
            if (isa<VarDecl>(parentDecl) ||
                isa<FieldDecl>(parentDecl) ||
                isa<ParmVarDecl>(parentDecl)) {
                return false;
            }
        }
        
        return true;
    }
    
    ASTContext *m_context;
    DiagnosticsEngine *m_diagEngine;
    unsigned m_diagMissingAttr;
    unsigned m_diagIgnoredResult;
};

class NodiscardConsumer : public ASTConsumer {
public:
    explicit NodiscardConsumer(ASTContext *context) 
        : m_visitor(context) {}

    void HandleTranslationUnit(ASTContext &context) override {
        m_visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    NodiscardVisitor m_visitor;
};

class NodiscardAction : public PluginASTAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci, 
                                                    llvm::StringRef inFile) override {
        return std::make_unique<NodiscardConsumer>(&ci.getASTContext());
    }

    bool ParseArgs(const CompilerInstance &ci,
                   const std::vector<std::string> &args) override {
        return true;
    }
};

} // namespace

static FrontendPluginRegistry::Add<NodiscardAction>
    X("zhulin_task1_plugin", "Check missing [[nodiscard]] and ignored results");

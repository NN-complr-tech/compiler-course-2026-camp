#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Attr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Basic/Diagnostic.h"

namespace {

class NoDiscardVisitor final
    : public clang::RecursiveASTVisitor<NoDiscardVisitor> {
public:

    explicit NoDiscardVisitor(clang::ASTContext &context)
        : Context(context),
          Diagnostics(context.getDiagnostics()) {

            // while creating visitor, we are creating a new diagnostic message -  with DiagnosticsEngine we make a new ID
            // and give it to our visitor
        WarningID = Diagnostics.getCustomDiagID(
            clang::DiagnosticsEngine::Warning,
            "function '%0' returns a value and should be marked [[nodiscard]]");

        IgnoredResultWarningID = Diagnostics.getCustomDiagID(
            clang::DiagnosticsEngine::Warning,
            "result of function '%0' is ignored");

    }

//__HELPER FUNCTION___
bool needsNoDiscard(const clang::FunctionDecl *function) {

    // already has [[nodiscard]]
    if (function->hasAttr<clang::WarnUnusedResultAttr>())
        return false;

    // returns void
    if (function->getReturnType()->isVoidType())
        return false;

    // constructors / destructors
    if (clang::isa<clang::CXXConstructorDecl>(function))
        return false;
    if (clang::isa<clang::CXXDestructorDecl>(function))
        return false;

    // conversion operators: operator int(), operator bool(), ...
    if (clang::isa<clang::CXXConversionDecl>(function))
        return false;

    // skip only operators that almost never need [[nodiscard]]
    if (function->isOverloadedOperator()) {
        switch (function->getOverloadedOperator()) {

        // assignment
        case clang::OO_Equal:

        // stream operators
        case clang::OO_LessLess:
        case clang::OO_GreaterGreater:

        // compound assignment
        case clang::OO_PlusEqual:
        case clang::OO_MinusEqual:
        case clang::OO_StarEqual:
        case clang::OO_SlashEqual:
        case clang::OO_PercentEqual:
        case clang::OO_AmpEqual:
        case clang::OO_PipeEqual:
        case clang::OO_CaretEqual:
        case clang::OO_LessLessEqual:
        case clang::OO_GreaterGreaterEqual:
            return false;

        default:
            break;
        }
    }

    return true;
}


//______DECLARATION VISIT____

bool VisitFunctionDecl(clang::FunctionDecl *function) {

    // only definitions
    if (!function->isThisDeclarationADefinition())
        return true;

    // function does not need [[nodiscard]] - in the helper func
    if (!needsNoDiscard(function))
        return true;

        // found warning - sending it it our diagnostic engine
    Diagnostics.Report(function->getLocation(), WarningID)
        << function->getQualifiedNameAsString();

    return true;
}


//__CALL VISIT___
bool VisitCallExpr(clang::CallExpr *call) {

    // first of all - find the function
    const clang::FunctionDecl *function = call->getDirectCallee();

    if (!function) return true;

    if (!needsNoDiscard(function))
        return true;

    auto parents = Context.getParents(*call);

    if (parents.empty()) return true;

    const clang::Stmt *parent = parents[0].get<clang::Stmt>();

    if (!parent) return true;


    // skip simple wrappers (tecnical nodes that clang creates - there will be more sometimes... thats just an MVP :) )
    while (clang::isa<clang::ImplicitCastExpr>(parent) ||
           clang::isa<clang::ParenExpr>(parent) ||
           clang::isa<clang::ExprWithCleanups>(parent)) {

            // we are like "climbing" on the tree while searching REAL parent
        auto next = Context.getParents(*parent);
        if (next.empty())
            return true;
        parent = next[0].get<clang::Stmt>();
        if (!parent) return true;
    }


    // result is ignored - warning!!!
    if (clang::isa<clang::CompoundStmt>(parent)) {

        Diagnostics.Report(
            call->getExprLoc(),
            IgnoredResultWarningID)
            << function->getQualifiedNameAsString();
    }

    return true;
}

private:
// for woking nodiscard we will need:

    clang::ASTContext &Context; // as usual, context. we need it almost everywhere
    // cause for example, diagnostic engine, source manager, types etc. are stocked there

    clang::DiagnosticsEngine &Diagnostics; // Using to report warnings and errors

    unsigned WarningID; // we will create new warnings, then we will need its ID
    unsigned IgnoredResultWarningID;
};

// the main purpose of this class is just to call our visitor is the AST is ready (if i understood it right)
// then it almost not changing from example
class NoDiscardConsumer : public clang::ASTConsumer {
public:
    explicit NoDiscardConsumer(clang::ASTContext &context)
        : Visitor(context) {}

    void HandleTranslationUnit(clang::ASTContext &context) override {
        // visitor, start traversing the AST!
        Visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

private:
    NoDiscardVisitor Visitor;
};
// something like a "entry point", factory. it tells clang whick consumer should it create
// again, not much changed
class NoDiscardAction : public clang::PluginASTAction {
public:
    std::unique_ptr<clang::ASTConsumer>

    CreateASTConsumer(clang::CompilerInstance &compiler,
                      llvm::StringRef) override {
        // create NoDiscardConsumer!
        return std::make_unique<NoDiscardConsumer>(
            compiler.getASTContext());
    }

    bool ParseArgs(const clang::CompilerInstance &,
                   const std::vector<std::string> &) override {
        return true;
    }
};

} // namespace

// Register the plugin in Clang
static clang::FrontendPluginRegistry::Add<NoDiscardAction>
    X("nodiscard-checker" // name of the plugin that we should register
        // cause later we will tell tests something like -plugin nodiscard-checker
        ,
      "Suggest [[nodiscard]] for functions returning values");
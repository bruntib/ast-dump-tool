#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Index/USRGeneration.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

class TreeBuilder : public clang::RecursiveASTVisitor<TreeBuilder>
{
public:
  explicit TreeBuilder(clang::ASTContext* Context)
    : Context(Context), depth(-1)
  {
  }

  bool shouldVisitImplicitCode() const
  {
    return true;
  }

  bool TraverseDecl(clang::Decl* decl)
  {
    ++depth;
    bool result = clang::RecursiveASTVisitor<TreeBuilder>::TraverseDecl(decl);
    --depth;

    return result;
  }

  bool TraverseStmt(clang::Stmt* stmt)
  {
    ++depth;
    bool result = clang::RecursiveASTVisitor<TreeBuilder>::TraverseStmt(stmt);
    --depth;

    return result;
  }

  bool VisitDecl(clang::Decl* decl)
  {
    indent();

    clang::SourceLocation loc = decl->getBeginLoc();
    clang::SourceManager& sm = Context->getSourceManager();

    llvm::outs()
      << decl->getDeclKindName() << ' '
      << getUSR(decl) << ' '
      << sm.getFilename(loc).str() << ':'
      << sm.getSpellingLineNumber(loc) << ':'
      << sm.getSpellingColumnNumber(loc) << ' '
      << (decl->isImplicit() ? "(implicit)" : "") << '\n';

    return true;
  }

  bool VisitStmt(clang::Stmt* stmt)
  {
    indent();

    clang::SourceLocation loc = stmt->getBeginLoc();
    clang::SourceManager& sm = Context->getSourceManager();

    llvm::outs()
      << stmt->getStmtClassName() << ' '
      << sm.getFilename(loc).str() << ':'
      << sm.getSpellingLineNumber(loc) << ':'
      << sm.getSpellingColumnNumber(loc) << '\n';

    return true;
  }

private:
  void indent() const
  {
    for (int i = 0; i < depth; ++i)
      llvm::outs() << ' ';
  }

  std::string getUSR(clang::Decl* decl) const
  {
    llvm::SmallVector<char> Usr;
    clang::index::generateUSRForDecl(decl, Usr);
    char* data = Usr.data();
    return std::string(data, data + Usr.size());
  }

  clang::ASTContext* Context;
  int depth;
};

class MyASTConsumer : public clang::ASTConsumer
{
public:
  explicit MyASTConsumer(clang::ASTContext* Context) : Visitor(Context)
  {
  }

  virtual void HandleTranslationUnit(clang::ASTContext &Context)
  {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  TreeBuilder Visitor;
};

class MyFrontendAction : public clang::ASTFrontendAction
{
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance& Compiler,
    llvm::StringRef InFile) override
  {
    return std::make_unique<MyASTConsumer>(&Compiler.getASTContext());
  }
};

int main(int argc, const char* argv[])
{
  llvm::cl::OptionCategory MyToolCategory("my-tool options");

  auto ExpectedParser =
    clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory);

  if (!ExpectedParser)
  {
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }

  clang::tooling::CommonOptionsParser& OptionsParser = ExpectedParser.get();
  clang::tooling::ClangTool Tool(
    OptionsParser.getCompilations(),
    OptionsParser.getSourcePathList());

  std::vector<std::string> Args{
    "-I/usr/lib/clang/18/include",
    "-I/usr/local/include",
    "-I/usr/include"
  };

  Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
    Args, clang::tooling::ArgumentInsertPosition::END));

  return Tool.run(
    clang::tooling::newFrontendActionFactory<MyFrontendAction>().get());
}
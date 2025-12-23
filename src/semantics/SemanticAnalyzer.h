#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <map>

#include "../parser/AST.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

enum class TypeKind {
  INT,
  CHAR,
  DOUBLE,
  STRING,
  VOID,
  UNKNOWN
};

struct Symbol {
  std::string name;
  TypeKind type;          // return type
  bool isArray = false, isFunction = false;
  std::vector<TypeKind> paramTypes;
  std::vector<bool> paramIsArray;
};

class Scope {
 public:
  Scope *parent;
  std::map<std::string, Symbol> symbols;

  explicit Scope(Scope *parent = nullptr) : parent(parent) {}

  bool Declare(const Symbol &sym);
  std::optional<Symbol> Lookup(const std::string &name);
};

class SemanticAnalyzer {
 public:
  void Analyze(ASTNode *root);

  [[nodiscard]] const std::vector<std::string> &GetErrors() const { return errors_; }

 private:
  Scope *currentScope = nullptr;
  std::vector<std::string> errors_;

  bool inFunction = false;
  // for tracking loops
  int loopDepth = 0;

  // track current function return type
  TypeKind currentReturnType = TypeKind::VOID;

  // internal helpers
  void EnterScope();
  void ExitScope();

  void Error(const std::string &msg);

  // type checking
  TypeKind NodeToType(const ASTNode *t);
  TypeKind CheckExpression(const ASTNode *node);
  void CheckStatement(const ASTNode *node);
  void CheckFunction(ASTNode *node);

  void DeclareVar(const ASTNode *v, TypeKind t);
  void CheckVarDeclList(const ASTNode *node);

  void CollectArgs(ASTNode* n, std::vector<ASTNode*>& out);
};

#endif

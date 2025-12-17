#include "SemanticAnalyzer.h"
#include <sstream>

// =============================
// Scope
// =============================

bool Scope::Declare(const Symbol &sym) {
  if (symbols.count(sym.name))
    return false;
  symbols[sym.name] = sym;
  return true;
}

std::optional<Symbol> Scope::Lookup(const std::string &name) {
  if (symbols.count(name))
    return symbols[name];
  if (parent)
    return parent->Lookup(name);
  return std::nullopt;
}

// =============================
// SemanticAnalyzer
// =============================

void SemanticAnalyzer::Error(const std::string &msg) {
  errors_.push_back(msg);
}

void SemanticAnalyzer::EnterScope() {
  currentScope = new Scope(currentScope);
}

void SemanticAnalyzer::ExitScope() {
  Scope *old = currentScope;
  currentScope = currentScope->parent;
  delete old;
}

TypeKind SemanticAnalyzer::NodeToType(const ASTNode *t) {
  if (!t)
    return TypeKind::UNKNOWN;

  if (t->kind == NodeKind::TypeNode) {
    if (t->text == "int") return TypeKind::INT;
    if (t->text == "char") return TypeKind::CHAR;
    if (t->text == "double") return TypeKind::DOUBLE;
    if (t->text == "string") return TypeKind::STRING;
    if (t->text == "void") return TypeKind::VOID;
  }

  return TypeKind::UNKNOWN;
}

void SemanticAnalyzer::Analyze(ASTNode *root) {
  currentScope = new Scope(nullptr);

  // pre-declare functions (first pass)
  for (auto &child: root->children) {
    if (child->kind == NodeKind::Function) {
      auto fnName = child->text;
      auto type = NodeToType(child->children[0].get());
      Symbol sym{fnName, type, false};

      if (!currentScope->Declare(sym))
        Error("Duplicate function: " + fnName);
    }
  }

  // second pass analysis
  for (auto &child: root->children) {
    if (child->kind == NodeKind::Function)
      CheckFunction(child.get());
    else
      CheckStatement(child.get());
  }

  ExitScope();
}

void SemanticAnalyzer::CheckFunction(ASTNode *node) {
  EnterScope();

  currentReturnType = NodeToType(node->children[0].get());

  // args are children 1...n-2, last child is body
  for (size_t i = 1; i + 1 < node->children.size(); i++) {
    auto *arg = node->children[i].get();
    TypeKind t = NodeToType(arg->children[0].get());
    DeclareVar(arg, t);
  }

  // body
  CheckStatement(node->children.back().get());

  ExitScope();
  currentReturnType = TypeKind::VOID;
}

void SemanticAnalyzer::DeclareVar(const ASTNode *v, TypeKind t) {
  Symbol sym{v->text, t, v->isArray};

  if (!currentScope->Declare(sym))
    Error("Duplicate variable: " + v->text);
}

void SemanticAnalyzer::CheckVarDeclList(const ASTNode *node) {
  auto typeNode = node->children[0].get();
  TypeKind t = NodeToType(typeNode);

  for (size_t i = 1; i < node->children.size(); i++) {
    auto *var = node->children[i].get();
    DeclareVar(var, t);

    if (!var->children.empty()) {
      CheckExpression(var->children[0].get());
    }
  }
}

// =============================
// Expression
// =============================

TypeKind SemanticAnalyzer::CheckExpression(const ASTNode *node) {
  switch (node->kind) {

    case NodeKind::Number:
      return TypeKind::INT;

    case NodeKind::String:
      return TypeKind::STRING;

    case NodeKind::Identifier: {
      auto s = currentScope->Lookup(node->text);
      if (!s) {
        Error("Undeclared variable: " + node->text);
        return TypeKind::UNKNOWN;
      }
      return s->type;
    }

    case NodeKind::Unary:
      return CheckExpression(node->children[0].get());

    case NodeKind::CommaExpr:
      CheckExpression(node->children[0].get());
      return CheckExpression(node->children[1].get());

    case NodeKind::Assign: {
      auto lhs = node->children[0].get();
      auto rhs = node->children[1].get();

      if (lhs->kind != NodeKind::Identifier && lhs->kind != NodeKind::Index)
        Error("Invalid assignment target");

      auto lt = CheckExpression(lhs);
      auto rt = CheckExpression(rhs);

      if (lt != rt && lt != TypeKind::UNKNOWN && rt != TypeKind::UNKNOWN)
        Error("Assignment type mismatch");

      return lt;
    }

    case NodeKind::Binary: {
      auto l = CheckExpression(node->children[0].get());
      auto r = CheckExpression(node->children[1].get());

      if (l == TypeKind::DOUBLE || r == TypeKind::DOUBLE)
        return TypeKind::DOUBLE;

      return TypeKind::INT;
    }

    case NodeKind::Index: {
      auto base = node->children[0].get();
      auto idx  = node->children[1].get();

      auto bt = CheckExpression(base);
      auto it = CheckExpression(idx);

      if (it != TypeKind::INT)
        Error("Array index must be int");

      if (base->kind == NodeKind::Identifier) {
        auto s = currentScope->Lookup(base->text);
        if (s && !s->isArray)
          Error("Indexing non-array variable: " + base->text);
      }

      return bt;
    }

    case NodeKind::Call: {
      auto callee = node->children[0].get();
      if (callee->kind != NodeKind::Identifier) {
        Error("Call target must be a function name");
        return TypeKind::UNKNOWN;
      }

      auto s = currentScope->Lookup(callee->text);
      if (!s) {
        Error("Call to undeclared function: " + callee->text);
        return TypeKind::UNKNOWN;
      }

      for (size_t i = 1; i < node->children.size(); i++)
        CheckExpression(node->children[i].get());

      return s->type;
    }

    default:
      return TypeKind::UNKNOWN;
  }
}

// =============================
// Statements
// =============================

void SemanticAnalyzer::CheckStatement(const ASTNode *node) {
  if (node->kind == NodeKind::ExprStmt) {
    node = node->children[0].get();
  }
  switch (node->kind) {
    case NodeKind::Block:
      EnterScope();
      for (auto &c: node->children)
        CheckStatement(c.get());
      ExitScope();
      return;

    case NodeKind::VarDeclList:
      CheckVarDeclList(node);
      return;

    case NodeKind::Return: {
      if (!node->children.empty()) {
        auto t = CheckExpression(node->children[0].get());
        if (t != currentReturnType)
          Error("Return type mismatch");
      } else if (currentReturnType != TypeKind::VOID)
        Error("Missing return value");

      return;
    }

    case NodeKind::Break:
    case NodeKind::Continue:
      if (loopDepth <= 0)
        Error("break/continue outside loop");
      return;

    case NodeKind::While:
    case NodeKind::DoWhile:
    case NodeKind::For:
      loopDepth++;
      for (auto &c: node->children)
        CheckStatement(c.get());
      loopDepth--;
      return;

    case NodeKind::ElseIf:
    case NodeKind::If:
      for (auto &c: node->children)
        CheckStatement(c.get());
      return;

    // Expression statements
    case NodeKind::Assign:
    case NodeKind::Binary:
    case NodeKind::Call:
      CheckExpression(node);
      return;

    default:
      return;
  }
}

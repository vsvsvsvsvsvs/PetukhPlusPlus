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

// Flattens CommaExpr tree to plain argument list.
// Example:
//    CommaExpr( CommaExpr(3, 7), 8 )
// → [3, 7, 8]
void SemanticAnalyzer::CollectArgs(ASTNode *node,
                                   std::vector<ASTNode *> &out) {
  if (node == nullptr) return;

  if (node->kind == NodeKind::CommaExpr) {
    // CommaExpr is always binary in your grammar
    if (node->children.size() == 2) {
      CollectArgs(node->children[0].get(), out);
      CollectArgs(node->children[1].get(), out);
    }
    return;
  }

  out.push_back(node);
}

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
      Symbol sym{fnName, type, false, true};
      for (size_t i = 1; i + 1 < child->children.size(); i++) {
        auto *arg = child->children[i].get();
        sym.paramTypes.push_back(NodeToType(arg->children[0].get()));
        sym.paramIsArray.push_back(arg->isArray);
      }
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
  inFunction = true;
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
  inFunction = false;
}

void SemanticAnalyzer::DeclareVar(const ASTNode *v, TypeKind t) {
  Symbol sym{v->text, t, v->isArray, false};

  if (!currentScope->Declare(sym))
    Error("Duplicate variable: " + v->text);
}

void SemanticAnalyzer::CheckVarDeclList(const ASTNode *node) {
  auto typeNode = node->children[0].get();
  TypeKind declared = NodeToType(typeNode);

  for (size_t i = 1; i < node->children.size(); i++) {
    auto *var = node->children[i].get();
    DeclareVar(var, declared);

    if (!var->children.empty()) {
      auto initType = CheckExpression(var->children[0].get());

      if (declared == TypeKind::UNKNOWN || initType == TypeKind::UNKNOWN)
        continue;

      // // ---- STRING: строго только string ----
      // if (declared == TypeKind::STRING || initType == TypeKind::STRING) {
      //   if (declared != TypeKind::STRING || initType != TypeKind::STRING)
      //     Error("cannot assign non-string to string");
      //   continue;
      // }

      // ---- обычная строгая проверка ----
      if (declared != initType)
        Error("initializer type mismatch");
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
      if (s->isFunction) {
        Error("Function used as value: " + node->text);
      }
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

      if (l == TypeKind::UNKNOWN || r == TypeKind::UNKNOWN)
        return TypeKind::UNKNOWN;

      const std::string &op = node->text;

      // ---------- RELATIONAL + EQUALITY ----------
      if (op == "<" || op == "<=" || op == ">" || op == ">=" ||
          op == "==" || op == "!=") {
        // числа сравнивать можно
        if ((l == TypeKind::INT || l == TypeKind::DOUBLE) &&
            (r == TypeKind::INT || r == TypeKind::DOUBLE))
          return TypeKind::INT;

        // строки == и != можно, если надо — скажи, включу
        if (l == TypeKind::STRING && r == TypeKind::STRING && (op == "==" || op == "!="))
          return TypeKind::INT;

        Error("invalid operands to comparison operator");
        return TypeKind::UNKNOWN;
      }

      // ---------- STRING ----------
      if (l == TypeKind::STRING || r == TypeKind::STRING) {
        if (op == "+" && l == TypeKind::STRING && r == TypeKind::STRING)
          return TypeKind::STRING;

        Error("invalid binary operation with string");
        return TypeKind::UNKNOWN;
      }

      // ---------- ARITHMETIC ----------
      if ((l == TypeKind::INT || l == TypeKind::DOUBLE) &&
          (r == TypeKind::INT || r == TypeKind::DOUBLE)) {
        if (l == TypeKind::DOUBLE || r == TypeKind::DOUBLE)
          return TypeKind::DOUBLE;

        return TypeKind::INT;
      }

      Error("incompatible binary operand types");
      return TypeKind::UNKNOWN;
    }

    case NodeKind::Index: {
      auto base = node->children[0].get();
      auto idx = node->children[1].get();

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

    // ======================================
    //             FUNCTION CALL
    // ======================================
    case NodeKind::Call: {
      auto callee = node->children[0].get();

      if (callee->kind != NodeKind::Identifier) {
        Error("Call target must be a function name");
        return TypeKind::UNKNOWN;
      }

      auto s = currentScope->Lookup(callee->text);
      if (!s->isFunction) {
        Error("Call of non-function: " + callee->text);
      }
      if (!s) {
        Error("Call to undeclared function: " + callee->text);
        return TypeKind::UNKNOWN;
      }

      std::vector<ASTNode *> args;

      // ✔ старое поведение: дети [1..n-1] = аргументы
      // ✔ новое поведение: грамматика даёт один CommaExpr
      if (node->children.size() > 1) {
        auto argRoot = node->children[1].get();
        CollectArgs(const_cast<ASTNode *>(argRoot), args);
      }

      // ---------- CHECK ARG COUNT ----------
      if (args.size() != s->paramTypes.size()) {
        std::ostringstream oss;
        oss << "wrong number of arguments in call to "
            << callee->text
            << " (expected " << s->paramTypes.size()
            << ", got " << args.size() << ")";
        Error(oss.str());
      }

      // ---------- TYPE CHECK ----------
      for (size_t i = 0; i < args.size(); i++) {
        auto t = CheckExpression(args[i]);

        if (i < s->paramTypes.size() &&
            t != s->paramTypes[i] &&
            t != TypeKind::UNKNOWN) {
          std::ostringstream oss;
          oss << "argument " << (i + 1)
              << " type mismatch in call to "
              << callee->text;
          Error(oss.str());
        }
      }

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
      if (!inFunction) {
        Error("return outside of function");
        return;
      }
      if (!node->children.empty()) {
        auto t = CheckExpression(node->children[0].get());
        if (t != currentReturnType) {
          Error("Return type mismatch");
        }
      } else if (currentReturnType != TypeKind::VOID)
        Error("Missing return value");

      return;
    }

    case NodeKind::Break:
    case NodeKind::Continue:
      if (loopDepth <= 0)
        Error("break/continue outside loop");
      return;

    case NodeKind::While: {
      // children[0] — условие
      if (!node->children.empty()) {
        auto cond = CheckExpression(node->children[0].get());
        if (cond != TypeKind::INT &&
            cond != TypeKind::UNKNOWN)
        {
          Error("While condition must be int");
        }
      }

      loopDepth++;
      // тело — остальные узлы
      for (size_t i = 1; i < node->children.size(); i++)
        CheckStatement(node->children[i].get());
      loopDepth--;
      return;
    }

    case NodeKind::DoWhile: {
      // тело — все кроме последнего
      loopDepth++;
      if (!node->children.empty()) {
        for (size_t i = 0; i + 1 < node->children.size(); i++)
          CheckStatement(node->children[i].get());
      }
      loopDepth--;

      // условие — последний ребенок
      if (!node->children.empty()) {
        auto cond = CheckExpression(node->children.back().get());
        if (cond != TypeKind::INT &&
            cond != TypeKind::UNKNOWN)
        {
          Error("Do-while condition must be int");
        }
      }

      return;
    }

    case NodeKind::For: {
      /*
        Ожидаем:
          0 — init (stmt или expr stmt или decl)
          1 — condition (expr)
          2 — step (expr stmt)
          остальное — тело (обычно один Block)
      */

      // init — это statement
      if (node->children.size() > 0)
        CheckStatement(node->children[0].get());

      // condition — expression
      if (node->children.size() > 1) {
        auto cond = CheckExpression(node->children[1].get());
        if (cond != TypeKind::INT &&
            cond != TypeKind::UNKNOWN)
        {
          Error("For condition must be int");
        }
      }

      // step — expression / exprstmt
      if (node->children.size() > 2)
        CheckStatement(node->children[2].get());

      // тело
      loopDepth++;
      for (size_t i = 3; i < node->children.size(); i++)
        CheckStatement(node->children[i].get());
      loopDepth--;

      return;
    }
    case NodeKind::If:
    case NodeKind::ElseIf: {
      if (!node->children.empty()) {
        auto condType = CheckExpression(node->children[0].get());

        if (condType != TypeKind::INT &&
            condType != TypeKind::UNKNOWN)
        {
          Error("If condition must be int");
        }
      }

      for (auto &c: node->children)
        CheckStatement(c.get());
      return;
    }

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

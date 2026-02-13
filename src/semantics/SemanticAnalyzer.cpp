#include "SemanticAnalyzer.h"
#include <sstream>
#include <cctype>

// helper: is numeric literal floating (contains '.' or 'e' / 'E')
static bool IsFloatingLiteral(const std::string &s) {
  for (char c : s) {
    if (c == '.' || c == 'e' || c == 'E') return true;
  }
  return false;
}

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

  // --- predeclare builtin functions (I/O) ---
  {
    Symbol s;

    s = Symbol{"printInt", TypeKind::VOID, false, true};
    s.paramTypes = { TypeKind::INT }; s.paramIsArray = { false };
    currentScope->Declare(s);

    s = Symbol{"printDouble", TypeKind::VOID, false, true};
    s.paramTypes = { TypeKind::DOUBLE }; s.paramIsArray = { false };
    currentScope->Declare(s);

    s = Symbol{"printStr", TypeKind::VOID, false, true};
    s.paramTypes = { TypeKind::STRING }; s.paramIsArray = { false };
    currentScope->Declare(s);

    s = Symbol{"inputInt", TypeKind::INT, false, true};
    s.paramTypes = {}; s.paramIsArray = {};
    currentScope->Declare(s);

    s = Symbol{"inputDouble", TypeKind::DOUBLE, false, true};
    s.paramTypes = {}; s.paramIsArray = {};
    currentScope->Declare(s);

    s = Symbol{"inputStr", TypeKind::STRING, false, true};
    s.paramTypes = {}; s.paramIsArray = {};
    currentScope->Declare(s);
  }

  // pre-declare functions (first pass) from AST
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

      // allow implicit int -> double conversion for initializer
      if (declared == initType) continue;
      if (declared == TypeKind::DOUBLE && initType == TypeKind::INT) continue;

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
      // detect floating literal
      if (IsFloatingLiteral(node->text)) return TypeKind::DOUBLE;
      return TypeKind::INT;

    case NodeKind::String:
      return TypeKind::STRING;

    case NodeKind::Identifier: {
      auto opt = currentScope->Lookup(node->text);
      if (!opt) {
        Error("Undeclared variable: " + node->text);
        return TypeKind::UNKNOWN;
      }
      Symbol s = *opt;
      if (s.isFunction) {
        Error("Function used as value: " + node->text);
        return TypeKind::UNKNOWN;
      }
      return s.type;
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

      // allow int -> double
      if (lt != rt && !(lt == TypeKind::DOUBLE && rt == TypeKind::INT)
          && lt != TypeKind::UNKNOWN && rt != TypeKind::UNKNOWN)
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
        // numeric comparisons allowed between int/double
        if ((l == TypeKind::INT || l == TypeKind::DOUBLE) &&
            (r == TypeKind::INT || r == TypeKind::DOUBLE))
          return TypeKind::INT;

        // string == and != allowed
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
        auto sOpt = currentScope->Lookup(base->text);
        if (sOpt && !sOpt->isArray)
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

      auto sOpt = currentScope->Lookup(callee->text);
      if (!sOpt) {
        Error("Call to undeclared function: " + callee->text);
        return TypeKind::UNKNOWN;
      }
      Symbol s = *sOpt;
      if (!s.isFunction) {
        Error("Call of non-function: " + callee->text);
        return TypeKind::UNKNOWN;
      }

      std::vector<ASTNode *> args;

      // old behaviour: children [1..n-1] = arguments
      if (node->children.size() > 1) {
        auto argRoot = node->children[1].get();
        CollectArgs(const_cast<ASTNode *>(argRoot), args);
      }

      // ---------- CHECK ARG COUNT ----------
      if (args.size() != s.paramTypes.size()) {
        std::ostringstream oss;
        oss << "wrong number of arguments in call to "
            << callee->text
            << " (expected " << s.paramTypes.size()
            << ", got " << args.size() << ")";
        Error(oss.str());
      }

      // ---------- TYPE CHECK ----------
      for (size_t i = 0; i < args.size(); i++) {
        auto t = CheckExpression(args[i]);

        if (i < s.paramTypes.size()) {
          // allow implicit int -> double conversion for params
          if (t != s.paramTypes[i] &&
              !(s.paramTypes[i] == TypeKind::DOUBLE && t == TypeKind::INT) &&
              t != TypeKind::UNKNOWN) {
            std::ostringstream oss;
            oss << "argument " << (i + 1)
                << " type mismatch in call to "
                << callee->text;
            Error(oss.str());
          }
        }
      }

      return s.type;
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
        if (t != currentReturnType &&
            !(currentReturnType == TypeKind::DOUBLE && t == TypeKind::INT)) {
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
        AST Structure (guaranteed by new parser):
          - children[0]: init statement (can be null)
          - children[1]: condition expression (can be null)
          - children[2]: step expression (can be null)
          - children[3]: body statement (can be null)
      */
      EnterScope(); // For-loops have their own scope (e.g., for 'int i=0')

      // 1. Init statement
      if (node->children[0]) {
        CheckStatement(node->children[0].get());
      }

      // 2. Condition expression
      if (node->children[1]) {
        auto condType = CheckExpression(node->children[1].get());
        if (condType != TypeKind::INT && condType != TypeKind::UNKNOWN) {
          Error("For loop condition must be an integer expression");
        }
      }

      // 3. Step expression (checked as an expression, value is discarded)
      if (node->children[2]) {
        CheckExpression(node->children[2].get());
      }

      // 4. Body
      loopDepth++;
      if (node->children[3]) {
        CheckStatement(node->children[3].get());
      }
      loopDepth--;

      ExitScope(); // Exit the for-loop's scope
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

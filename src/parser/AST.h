#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

enum class NodeKind {
  Program,
  Function,
  FuncArg,
  Block,
  VarDeclList,
  VarDecl,
  If,
  ElseIf,
  While,
  DoWhile,
  For,
  Return,
  Break,
  Continue,
  ExprStmt,

  // Expressions
  Assign,
  CommaExpr,
  Binary,
  Unary,
  Number,
  String,
  Identifier,
  Call,
  Index,
  TypeNode
};

inline const char *NodeKindToString(NodeKind k) {
  switch (k) {
    case NodeKind::Program: return "Program";
    case NodeKind::Function: return "Function";
    case NodeKind::FuncArg: return "FuncArg";
    case NodeKind::Block: return "Block";
    case NodeKind::VarDeclList: return "VarDeclList";
    case NodeKind::VarDecl: return "VarDecl";
    case NodeKind::If: return "If";
    case NodeKind::ElseIf: return "ElseIf";
    case NodeKind::While: return "While";
    case NodeKind::DoWhile: return "DoWhile";
    case NodeKind::For: return "For";
    case NodeKind::Return: return "Return";
    case NodeKind::Break: return "Break";
    case NodeKind::Continue: return "Continue";
    case NodeKind::ExprStmt: return "ExprStmt";

    case NodeKind::Assign: return "Assign";
    case NodeKind::CommaExpr: return "CommaExpr";
    case NodeKind::Binary: return "Binary";
    case NodeKind::Unary: return "Unary";
    case NodeKind::Number: return "Number";
    case NodeKind::String: return "String";
    case NodeKind::Identifier: return "Identifier";
    case NodeKind::Call: return "Call";
    case NodeKind::Index: return "Index";
    case NodeKind::TypeNode: return "Type";
    default: return "Unknown";
  }
}

struct ASTNode {
  NodeKind kind;
  std::string text;
  bool isArray = false;

  std::vector<std::unique_ptr<ASTNode> > children;
};

inline std::unique_ptr<ASTNode> make_node(NodeKind kind, const std::string &text = "") {
  auto n = std::make_unique<ASTNode>();
  n->kind = kind;
  n->text = text;
  return n;
}

#endif // AST_H

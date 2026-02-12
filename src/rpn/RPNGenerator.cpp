#include "RPNGenerator.h"

std::vector<Instruction> RPNGenerator::Generate(ASTNode *root) {
  code_.clear();
  labelCounter_ = 0;
  breakLabels_.clear();
  continueLabels_.clear();

  if (!root)
    return code_;

  for (auto &child: root->children)
    GenNode(child.get());

  return code_;
}

std::string RPNGenerator::NewLabel() {
  return "L" + std::to_string(labelCounter_++);
}

void RPNGenerator::GenNode(ASTNode *node) {
  if (!node)
    return;

  if (node->kind == NodeKind::Function)
    GenFunction(node);
  else
    GenStatement(node);
}

void RPNGenerator::GenFunction(ASTNode *node) {
  code_.emplace_back(OpCode::LABEL, node->text);

  for (auto &child: node->children)
    GenStatement(child.get());

  // добавляем RET только если нет явного return
  if (code_.empty() || code_.back().op != OpCode::RET)
    code_.emplace_back(OpCode::RET);
}

void RPNGenerator::GenStatement(ASTNode *node) {
  if (!node)
    return;

  switch (node->kind) {
    case NodeKind::Block:
      for (auto &stmt: node->children)
        GenStatement(stmt.get());
      break;

    case NodeKind::ExprStmt:
      if (!node->children.empty())
        GenExpression(node->children[0].get());
      break;

    case NodeKind::VarDeclList:
      // children[0] — TypeNode
      for (size_t i = 1; i < node->children.size(); ++i) {
        auto *var = node->children[i].get();

        if (!var->children.empty()) {
          GenExpression(var->children[0].get());
          code_.emplace_back(OpCode::STORE, var->text);
        }
      }
      break;

    case NodeKind::Assign:
      GenExpression(node->children[1].get());
      code_.emplace_back(OpCode::STORE,
                         node->children[0]->text);
      break;

    case NodeKind::If:
      GenIf(node);
      break;

    case NodeKind::While: {
      std::string start = NewLabel();
      std::string end = NewLabel();

      breakLabels_.push_back(end);
      continueLabels_.push_back(start);

      code_.emplace_back(OpCode::LABEL, start);

      GenExpression(node->children[0].get());
      code_.emplace_back(OpCode::JZ, end);

      GenStatement(node->children[1].get());
      code_.emplace_back(OpCode::JMP, start);

      code_.emplace_back(OpCode::LABEL, end);

      breakLabels_.pop_back();
      continueLabels_.pop_back();
      break;
    }

    case NodeKind::DoWhile: {
      std::string start = NewLabel();
      std::string end = NewLabel();

      breakLabels_.push_back(end);
      continueLabels_.push_back(start);

      code_.emplace_back(OpCode::LABEL, start);

      GenStatement(node->children[0].get());
      GenExpression(node->children[1].get());

      code_.emplace_back(OpCode::JZ, end);
      code_.emplace_back(OpCode::JMP, start);
      code_.emplace_back(OpCode::LABEL, end);

      breakLabels_.pop_back();
      continueLabels_.pop_back();
      break;
    }

    case NodeKind::For: {
      // children: init, cond, iter, body
      std::string start = NewLabel();
      std::string end = NewLabel();
      std::string iterLabel = NewLabel();

      breakLabels_.push_back(end);
      continueLabels_.push_back(iterLabel);

      GenStatement(node->children[0].get());

      code_.emplace_back(OpCode::LABEL, start);

      GenExpression(node->children[1].get());
      code_.emplace_back(OpCode::JZ, end);

      GenStatement(node->children[3].get());

      code_.emplace_back(OpCode::LABEL, iterLabel);
      GenExpression(node->children[2].get());
      code_.emplace_back(OpCode::JMP, start);

      code_.emplace_back(OpCode::LABEL, end);

      breakLabels_.pop_back();
      continueLabels_.pop_back();
      break;
    }

    case NodeKind::Break:
      if (!breakLabels_.empty())
        code_.emplace_back(OpCode::JMP, breakLabels_.back());
      break;

    case NodeKind::Continue:
      if (!continueLabels_.empty())
        code_.emplace_back(OpCode::JMP, continueLabels_.back());
      break;

    case NodeKind::Return:
      if (!node->children.empty())
        GenExpression(node->children[0].get());
      code_.emplace_back(OpCode::RET);
      break;

    default:
      break;
  }
}

void RPNGenerator::GenIf(ASTNode *node) {
  std::string endLabel = NewLabel();
  std::string nextLabel;

  // --- основной if ---
  nextLabel = NewLabel();

  GenExpression(node->children[0].get());
  code_.emplace_back(OpCode::JZ, nextLabel);

  GenStatement(node->children[1].get());
  code_.emplace_back(OpCode::JMP, endLabel);

  code_.emplace_back(OpCode::LABEL, nextLabel);

  // --- обработка остальных детей ---
  for (size_t i = 2; i < node->children.size(); ++i) {
    ASTNode *child = node->children[i].get();

    // else-if
    if (child->kind == NodeKind::ElseIf) {
      std::string elseIfNext = NewLabel();

      GenExpression(child->children[0].get());
      code_.emplace_back(OpCode::JZ, elseIfNext);

      GenStatement(child->children[1].get());
      code_.emplace_back(OpCode::JMP, endLabel);

      code_.emplace_back(OpCode::LABEL, elseIfNext);
    } else {
      // обычный else
      GenStatement(child);
    }
  }

  code_.emplace_back(OpCode::LABEL, endLabel);
}


void RPNGenerator::GenExpression(ASTNode *node) {
  if (!node)
    return;

  switch (node->kind) {
    case NodeKind::Number:
      code_.emplace_back(OpCode::PUSH_INT, node->text);
      break;

    case NodeKind::String:
      code_.emplace_back(OpCode::PUSH_STRING, node->text);
      break;

    case NodeKind::Identifier:
      code_.emplace_back(OpCode::LOAD, node->text);
      break;

    case NodeKind::Unary:
      GenExpression(node->children[0].get());
      if (node->text == "-")
        code_.emplace_back(OpCode::NEG);
      else if (node->text == "!")
        code_.emplace_back(OpCode::NOT);
      break;

    case NodeKind::Binary:
      GenExpression(node->children[0].get());
      GenExpression(node->children[1].get());

      if (node->text == "+") code_.emplace_back(OpCode::ADD);
      else if (node->text == "-") code_.emplace_back(OpCode::SUB);
      else if (node->text == "*") code_.emplace_back(OpCode::MUL);
      else if (node->text == "/") code_.emplace_back(OpCode::DIV);
      else if (node->text == "==") code_.emplace_back(OpCode::EQ);
      else if (node->text == "!=") code_.emplace_back(OpCode::NEQ);
      else if (node->text == "<") code_.emplace_back(OpCode::LT);
      else if (node->text == ">") code_.emplace_back(OpCode::GT);
      else if (node->text == "<=") code_.emplace_back(OpCode::LE);
      else if (node->text == ">=") code_.emplace_back(OpCode::GE);
      break;

    case NodeKind::Call: {
      auto *callee = node->children[0].get();

      for (size_t i = 1; i < node->children.size(); ++i)
        GenExpression(node->children[i].get());

      code_.emplace_back(OpCode::CALL, callee->text);
      break;
    }

    case NodeKind::Index:
      GenExpression(node->children[0].get());
      GenExpression(node->children[1].get());
      code_.emplace_back(OpCode::LOAD_INDEX);
      break;

    case NodeKind::Assign:
      GenExpression(node->children[1].get());
      code_.emplace_back(OpCode::STORE,
                         node->children[0]->text);
      break;

    case NodeKind::CommaExpr:
      for (auto &ch: node->children)
        GenExpression(ch.get());
      break;

    default:
      break;
  }
}

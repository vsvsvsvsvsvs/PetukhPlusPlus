#include "RPNGenerator.h"
#include <cctype>

static bool IsFloatingLiteral(const std::string &s) {
  for (char c : s) {
    if (c == '.' || c == 'e' || c == 'E') return true;
  }
  return false;
}

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
  // function label
  code_.emplace_back(OpCode::LABEL, node->text);

  // collect parameter names (children[1..n-2]) — child 0 is return type, last child is body
  std::vector<std::string> paramNames;
  for (size_t i = 1; i + 1 < node->children.size(); ++i) {
    paramNames.push_back(node->children[i]->text);
  }

  // At function entry arguments are on the stack in call order (arg0, arg1, ...).
  // To bind them to local names we emit STORE for parameters in reverse order so
  // the top of stack (last argument) goes to the last parameter.
  for (int i = static_cast<int>(paramNames.size()) - 1; i >= 0; --i) {
    code_.emplace_back(OpCode::STORE, paramNames[i]);
  }

  // generate body (the last child)
  if (!node->children.empty()) {
    GenStatement(node->children.back().get());
  }

  // add RET if not present at the end
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
      if (!node->children.empty()) {
        auto *expr = node->children[0].get();
        GenExpression(expr);
        // Если выражение само не потребляет результат (CALL и ASSIGN — потребляют),
        // то нужно сбросить результат, иначе он накапливается на стеке.
        if (expr->kind != NodeKind::Call && expr->kind != NodeKind::Assign)
          code_.emplace_back(OpCode::POP);
      }
      break;


    case NodeKind::VarDeclList:
      // children[0] — TypeNode
      for (size_t i = 1; i < node->children.size(); ++i) {
        auto *var = node->children[i].get();

        if (var->isArray) {
          // array declaration: expect size expression; if missing use 0
          if (!var->children.empty()) {
            GenExpression(var->children[0].get());
          } else {
            code_.emplace_back(OpCode::PUSH_INT, "0");
          }
          code_.emplace_back(OpCode::NEW_ARRAY);
          code_.emplace_back(OpCode::STORE, var->text);
        } else {
          // scalar variable
          if (!var->children.empty()) {
            GenExpression(var->children[0].get());
          } else {
            // initialize plain variable with 0
            code_.emplace_back(OpCode::PUSH_INT, "0");
          }
          code_.emplace_back(OpCode::STORE, var->text);
        }
      }
      break;


    case NodeKind::Assign:
      // handled in GenExpression (assignment may appear inside expressions)
      // but if assignment appears as a statement we still need to generate it
      if (!node->children.empty())
        GenExpression(node);
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
      // AST: children[0]=init, children[1]=cond, children[2]=step, children[3]=body
      std::string startLabel = NewLabel();
      std::string stepLabel = NewLabel();
      std::string endLabel = NewLabel();

      breakLabels_.push_back(endLabel);
      continueLabels_.push_back(stepLabel);

      // 1. Init part
      if (node->children[0]) {
        GenStatement(node->children[0].get());
      }

      code_.emplace_back(OpCode::LABEL, startLabel);

      // 2. Condition part
      if (node->children[1]) {
        GenExpression(node->children[1].get());
        code_.emplace_back(OpCode::JZ, endLabel); // If condition is false, jump to end
      }

      // 4. Body part
      if (node->children[3]) {
        GenStatement(node->children[3].get());
      }

      // 3. Step part
      code_.emplace_back(OpCode::LABEL, stepLabel);
      if (node->children[2]) {
        auto* stepExpr = node->children[2].get();
        GenExpression(stepExpr);

        // Pop result of step expression if it's not an assignment or call
        if (stepExpr->kind != NodeKind::Call && stepExpr->kind != NodeKind::Assign) {
          code_.emplace_back(OpCode::POP);
        }
      }

      // Jump back to the start to re-evaluate the condition
      code_.emplace_back(OpCode::JMP, startLabel);

      code_.emplace_back(OpCode::LABEL, endLabel);

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
      if (IsFloatingLiteral(node->text))
        code_.emplace_back(OpCode::PUSH_DOUBLE, node->text);
      else
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
      else if (node->text == "%") code_.emplace_back(OpCode::MOD);
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
      // push array, then index, then LOAD_INDEX will consume them
      GenExpression(node->children[0].get());
      GenExpression(node->children[1].get());
      code_.emplace_back(OpCode::LOAD_INDEX);
      break;

    case NodeKind::Assign:
      // assignment could be either to identifier or to index
      // evaluate RHS first (so it sits under array/index if needed)
      if (node->children[0]->kind == NodeKind::Index) {
        // special-case: a simple variable index like pref[i]
        auto *idxNode = node->children[0].get();
        auto *arrExpr = idxNode->children[0].get();
        auto *indexExpr = idxNode->children[1].get();

        if (arrExpr->kind == NodeKind::Identifier) {
          // emit: RHS, index, STORE_INDEX <varname>
          GenExpression(node->children[1].get());   // RHS value
          GenExpression(indexExpr);                 // index
          code_.emplace_back(OpCode::STORE_INDEX, arrExpr->text);
        } else {
          // fallback to previous behaviour: rhs, array, index, STORE_INDEX
          GenExpression(node->children[1].get());
          GenExpression(arrExpr);
          GenExpression(indexExpr);
          code_.emplace_back(OpCode::STORE_INDEX);
        }
      } else {
        GenExpression(node->children[1].get());
        code_.emplace_back(OpCode::STORE, node->children[0]->text);
      }
      break;

    case NodeKind::CommaExpr:
      for (auto &ch: node->children)
        GenExpression(ch.get());
      break;

    default:
      break;
  }
}
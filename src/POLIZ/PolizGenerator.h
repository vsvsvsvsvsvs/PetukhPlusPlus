#pragma once

#include "AST.h"
#include "PolizInstruction.h"
#include <vector>
#include <string>

class PolizGenerator {
public:
  std::vector<Instruction> Generate(ASTNode* root);

private:
  std::vector<Instruction> code_;
  int labelCounter_ = 0;

  std::vector<std::string> breakLabels_;
  std::vector<std::string> continueLabels_;

  std::string NewLabel();

  void GenNode(ASTNode* node);
  void GenStatement(ASTNode* node);
  void GenExpression(ASTNode* node);
  void GenFunction(ASTNode* node);
  void GenIf(ASTNode* node);
};

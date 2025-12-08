#pragma once
#include "AST.h"
#include <iostream>
#include <ostream>

class ASTPrinter {
public:
  void Print(const ASTNode *node, std::ostream &out, int indent = 0);

private:
  void PrintIndent(std::ostream &out, int indent);
};

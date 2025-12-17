#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "AST.h"
#include <iostream>
#include <ostream>

class ASTPrinter {
 public:
  void Print(const ASTNode *node, std::ostream &out, int indent = 0);

 private:
  static void PrintIndent(std::ostream &out, int indent);
};

#endif // AST_PRINTER_H

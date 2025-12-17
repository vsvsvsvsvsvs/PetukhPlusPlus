#include "ASTPrinter.h"

void ASTPrinter::PrintIndent(std::ostream &out, int indent) {
  for (int i = 0; i < indent; ++i) out << "  ";
}

void ASTPrinter::Print(const ASTNode *node, std::ostream &out, int indent) {
  if (!node) return;

  PrintIndent(out, indent);

  out << NodeKindToString(node->kind);
  if (!node->text.empty()) {
    out << ": " << node->text;
  }
  if (node->isArray) {
    out << " [array]";
  }
  out << "\n";

  for (auto &child: node->children) {
    Print(child.get(), out, indent + 1);
  }
}

#include <iostream>
#include <fstream>
#include <sstream>
#include "Lexer.h"
#include "Parser.h"
#include "ASTPrinter.h"

int main() {
  const std::string programPath = "/root/CLionProjects/PetukhPlusPlus/program.petukh";
  const std::string lexerOutPath = "/root/CLionProjects/PetukhPlusPlus/res_lexer.txt";
  const std::string syntaxOutPath = "/root/CLionProjects/PetukhPlusPlus/res_syntax.txt";

  // ---------- Load source ----------
  std::ifstream fin(programPath);
  if (!fin.is_open()) {
    std::cerr << "Error: cannot open program.petukh\n";
    return 1;
  }

  std::stringstream buffer;
  buffer << fin.rdbuf();
  fin.close();

  std::string source = buffer.str();

  // ---------- Run lexer ----------
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.Tokenize();

  // Save lexer output
  {
    std::ofstream f(lexerOutPath);
    for (const auto &t : tokens) {
      f << "Line " << t.line << ":" << t.col << "  "
        << TokenTypeToString(t.type) << "  '" << t.text << "'\n";
    }
  }

  // ---------- Run parser ----------
  Parser parser(tokens);
  std::unique_ptr<ASTNode> program = parser.ParseProgram();

  // ---------- Save syntax output ----------
  std::ofstream fout(syntaxOutPath);

  // Print AST only if there are no syntax errors
  if (parser.errors_.empty()) {
    ASTPrinter printer;
    printer.Print(program.get(), fout);
    fout << "\n=== No syntax errors ===\n";
  } else {
    fout << "=== AST (partial or empty due to errors) ===\n";
    ASTPrinter printer;
    if (program)
      printer.Print(program.get(), fout);

    fout << "\n=== Syntax errors ===\n";
    for (const auto &err : parser.errors_) {
      fout << err << "\n";
    }

    std::cerr << "Syntax errors detected. See res_syntax.txt.\n";
  }

  return 0;
}

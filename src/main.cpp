#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "parser/ASTPrinter.h"
#include "semantics/SemanticAnalyzer.h"
#include "rpn/RPNGenerator.h"
#include "rpn/RPNInstruction.h"

int main() {
  const std::string programPath =
      "../examples/program.petukh";

  const std::string lexerOutPath =
      "../examples/res_lexer.txt";

  const std::string syntaxOutPath =
      "../examples/res_syntax.txt";

  const std::string semanticOutPath =
      "../examples/res_semantic.txt";

  const std::string polizOutPath =
      "../examples/res_poliz.txt";


  // ================= Load source =================
  std::ifstream fin(programPath);
  if (!fin.is_open()) {
    std::cerr << "Error: cannot open program.petukh\n";
    return 1;
  }

  std::stringstream buffer;
  buffer << fin.rdbuf();
  fin.close();

  std::string source = buffer.str();


  // ================= Lexer =================
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.Tokenize();

  {
    std::ofstream f(lexerOutPath);
    for (const auto &t : tokens) {
      f << "Line " << t.line << ":" << t.col << "  "
        << TokenTypeToString(t.type) << "  '" << t.text << "'\n";
    }
  }


  // ================= Parser =================
  Parser parser(tokens);
  std::unique_ptr<ASTNode> program = parser.ParseProgram();

  {
    std::ofstream fout(syntaxOutPath);

    if (parser.GetErrors().empty()) {
      ASTPrinter printer;
      printer.Print(program.get(), fout);
      fout << "\n=== No syntax errors ===\n";
    } else {
      fout << "=== AST (partial or empty due to errors) ===\n";
      ASTPrinter printer;
      if (program)
        printer.Print(program.get(), fout);

      fout << "\n=== Syntax errors ===\n";
      for (const auto &err : parser.GetErrors()) {
        fout << err << "\n";
      }

      std::cerr << "Syntax errors detected. See res_syntax.txt.\n";
      return 1;
    }
  }


  // ================= Semantic =================
  SemanticAnalyzer sema;
  sema.Analyze(program.get());

  const auto &semErrors = sema.GetErrors();

  {
    std::ofstream fs(semanticOutPath);

    if (semErrors.empty()) {
      fs << "=== Semantic OK ===\n";
      std::cout << "No semantic errors.\n";
    } else {
      fs << "=== Semantic errors ===\n";
      for (const auto &e : semErrors) {
        fs << e << "\n";
      }
      std::cerr << "Semantic errors detected. See res_semantic.txt.\n";
      return 1;
    }
  }


  // ================= POLIZ =================
  RPNGenerator generator;
  auto poliz = generator.Generate(program.get());

  {
    std::ofstream fp(polizOutPath);

    fp << "=== POLIZ ===\n\n";

    for (size_t i = 0; i < poliz.size(); ++i) {
      fp << i << ": "
         << OpCodeToString(poliz[i].op);

      if (!poliz[i].arg.empty())
        fp << " " << poliz[i].arg;

      fp << "\n";
    }
  }


  std::cout << "Compilation successful.\n";
  std::cout << "POLIZ written to res_poliz.txt\n";

  return 0;
}

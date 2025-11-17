main.cpp:

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Lexer.h"

std::string ReadFileToString(const std::string& filename) {
  std::ifstream file_stream(filename);
  if (!file_stream.is_open()) {
    std::cerr << "Error: Failed to open file \"" << filename << "\".\n";
    return "";
  }
  std::stringstream buffer;
  buffer << file_stream.rdbuf();
  return buffer.str();
}

int main() {
  const std::string filename = "../program.petukh";
  const std::string code = ReadFileToString(filename);
  if (code.empty()) {
    std::cerr << "Error: Source code is empty or file read failed." << "\n";
    return 1;
  }
  Lexer lexer(code);
  while (true) {
    Token token = lexer.NextToken();
    std::cout << static_cast<int>(token.type) << "  " << token.text << "\n";
    if (token.type == TokenType::END_OF_FILE) {
      break;
    }
  }
  return 0;
}

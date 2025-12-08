#include <iostream>
#include <fstream>
#include <sstream>
#include "Lexer.h"
#include "Parser.h"
#include "ASTPrinter.h"

// -------------------------------------------
//  Функция для вывода токенов в файл
// -------------------------------------------
const char* TokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_FOR: return "KW_FOR";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::KW_DO: return "KW_DO";
        case TokenType::KW_FN: return "KW_FN";
        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_DOUBLE: return "KW_DOUBLE";
        case TokenType::KW_STRING: return "KW_STRING";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_BREAK: return "KW_BREAK";
        case TokenType::KW_CONTINUE: return "KW_CONTINUE";

        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";

        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";

        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LE: return "LE";
        case TokenType::GE: return "GE";

        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";

        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";

        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

void DumpTokens(const std::vector<Token> &tokens, const std::string &path) {
    std::ofstream fout(path);
    if (!fout.is_open()) {
        std::cerr << "Error: cannot open " << path << "\n";
        return;
    }

    for (const auto &t : tokens) {
        fout << TokenTypeToString(t.type);
        if (!t.text.empty())
            fout << " : " << t.text;
        fout << "\n";
    }
}

// -------------------------------------------
//                    main
// -------------------------------------------

int main() {
    std::string inputPath = "/root/CLionProjects/PetukhPlusPlus/program.petukh";
    std::string lexOutPath = "/root/CLionProjects/PetukhPlusPlus/res_lexer.txt";
    std::string astOutPath = "/root/CLionProjects/PetukhPlusPlus/res_syntax.txt";

    std::ifstream fin(inputPath);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open program.petukh\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << fin.rdbuf();
    fin.close();

    std::string source = buffer.str();

    // 1) Лексер
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.Tokenize();

    // Вывод токенов
    DumpTokens(tokens, lexOutPath);

    // 2) Парсер
    Parser parser(tokens);
    std::unique_ptr<ASTNode> program;

    try {
        program = parser.ParseProgram();
    } catch (const std::exception &e) {
        std::ofstream ferr(astOutPath);
        ferr << "Compilation error: " << e.what() << "\n";
        return 1;
    }

    // 3) Печать AST
    ASTPrinter printer;
    std::ofstream fout(astOutPath);
    printer.Print(program.get(), fout);

    return 0;
}

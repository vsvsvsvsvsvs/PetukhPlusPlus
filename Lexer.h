#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "Token.h"
#include "Trie.h"

class Lexer {
 public:
  explicit Lexer(std::string src);

  std::vector<Token> Tokenize();
  Token NextToken();
  Token PeekToken();

 private:
  std::string src_;
  size_t pos_;
  int line_, col_;

  Trie keyword_trie_;

  Token buffered_token_;
  bool has_buffered_token_ = false;

  void InitTrie() const;

  [[nodiscard]] bool IsEnd() const;
  [[nodiscard]] char Peek(int offset = 0) const;
  char Get();

  void SkipWhitespace();

  static bool IsLetter(char c);

  Token Identifier();
  Token Number();
  Token StringLiteral();
  Token Symbol();
  [[nodiscard]] Token EndOfFile() const;

  [[nodiscard]] Token Make(TokenType type, const std::string &text,
                           int start_col) const;
};

static const char* TokenTypeToString(TokenType t) {
  switch (t) {
    case TokenType::KW_FN: return "KW_FN";
    case TokenType::KW_INT: return "KW_INT";
    case TokenType::KW_CHAR: return "KW_CHAR";
    case TokenType::KW_DOUBLE: return "KW_DOUBLE";
    case TokenType::KW_STRING: return "KW_STRING";

    case TokenType::KW_IF: return "KW_IF";
    case TokenType::KW_ELSE: return "KW_ELSE";
    case TokenType::KW_WHILE: return "KW_WHILE";
    case TokenType::KW_DO: return "KW_DO";
    case TokenType::KW_FOR: return "KW_FOR";
    case TokenType::KW_RETURN: return "KW_RETURN";
    case TokenType::KW_BREAK: return "KW_BREAK";
    case TokenType::KW_CONTINUE: return "KW_CONTINUE";

    case TokenType::IDENTIFIER: return "IDENTIFIER";
    case TokenType::NUMBER: return "NUMBER";
    case TokenType::STRING_LITERAL: return "STRING_LITERAL";

    case TokenType::PLUS: return "+";
    case TokenType::MINUS: return "-";
    case TokenType::STAR: return "*";
    case TokenType::SLASH: return "/";
    case TokenType::PERCENT: return "%";

    case TokenType::ASSIGN: return "=";
    case TokenType::EQ: return "==";
    case TokenType::NEQ: return "!=";
    case TokenType::LT: return "<";
    case TokenType::LE: return "<=";
    case TokenType::GT: return ">";
    case TokenType::GE: return ">=";

    case TokenType::LPAREN: return "(";
    case TokenType::RPAREN: return ")";
    case TokenType::LBRACE: return "{";
    case TokenType::RBRACE: return "}";
    case TokenType::LBRACKET: return "[";
    case TokenType::RBRACKET: return "]";
    case TokenType::SEMICOLON: return ";";
    case TokenType::COMMA: return ",";

    case TokenType::END_OF_FILE: return "EOF";
    default: return "UNKNOWN";
  }
}

#endif

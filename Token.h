#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
  KW_IF,
  KW_ELSE,
  KW_FOR,
  KW_WHILE,
  KW_DO,
  KW_FN,
  KW_INT,
  KW_CHAR,
  KW_DOUBLE,
  KW_STRING,
  KW_RETURN,
  KW_BREAK,
  KW_CONTINUE,

  IDENTIFIER,
  NUMBER,
  STRING_LITERAL,

  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,
  ASSIGN,
  EQ,
  NEQ,
  LT,
  GT,
  LE,
  GE,

  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  COMMA,
  SEMICOLON,

  END_OF_FILE,
  UNKNOWN
};

struct Token {
  TokenType type;
  std::string text;
  int line, col;
};

#endif

#include "Lexer.h"

#include <cctype>
#include <utility>

Lexer::Lexer(std::string src) : src_(std::move(src)), pos_(0), line_(1), col_(1) {
  InitTrie();
}

void Lexer::InitTrie() const {
  keyword_trie_.Insert("if", static_cast<int>(TokenType::KW_IF));
  keyword_trie_.Insert("else", static_cast<int>(TokenType::KW_ELSE));
  keyword_trie_.Insert("for", static_cast<int>(TokenType::KW_FOR));
  keyword_trie_.Insert("while", static_cast<int>(TokenType::KW_WHILE));
  keyword_trie_.Insert("do", static_cast<int>(TokenType::KW_DO));
  keyword_trie_.Insert("fn", static_cast<int>(TokenType::KW_FN));

  keyword_trie_.Insert("int", static_cast<int>(TokenType::KW_INT));
  keyword_trie_.Insert("char", static_cast<int>(TokenType::KW_CHAR));
  keyword_trie_.Insert("double", static_cast<int>(TokenType::KW_DOUBLE));
  keyword_trie_.Insert("string", static_cast<int>(TokenType::KW_STRING));

  keyword_trie_.Insert("return", static_cast<int>(TokenType::KW_RETURN));
  keyword_trie_.Insert("break", static_cast<int>(TokenType::KW_BREAK));
  keyword_trie_.Insert("continue", static_cast<int>(TokenType::KW_CONTINUE));
}

bool Lexer::IsEnd() const { return pos_ >= src_.size(); }

char Lexer::Peek(int offset) const {
  if (pos_ + offset >= src_.size()) {
    return '\0';
  }
  return src_[pos_ + offset];
}

char Lexer::Get() {
  const char c = Peek();
  pos_++;
  if (c == '\n') {
    line_++;
    col_ = 1;
  } else {
    col_++;
  }
  return c;
}

void Lexer::SkipWhitespace() {
  while (!IsEnd() && isspace(Peek())) {
    Get();
  }
}

bool Lexer::IsLetter(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<Token> Lexer::Tokenize() {
  std::vector<Token> tokens;
  Token t;
  while ((t = NextToken()).type != TokenType::END_OF_FILE) {
    tokens.push_back(t);
  }
  tokens.push_back(t);
  return tokens;
}

Token Lexer::PeekToken() {
  if (!has_buffered_token_) {
    buffered_token_ = NextToken();
    has_buffered_token_ = true;
  }
  return buffered_token_;
}

Token Lexer::NextToken() {
  if (has_buffered_token_) {
    has_buffered_token_ = false;
    return buffered_token_;
  }
  SkipWhitespace();
  if (IsEnd()) {
    return EndOfFile();
  }
  const char c = Peek();
  if (IsLetter(c)) {
    return Identifier();
  }
  if (isdigit(c)) {
    return Number();
  }
  if (c == '"') {
    return StringLiteral();
  }
  return Symbol();
}

Token Lexer::EndOfFile() const {
  return Make(TokenType::END_OF_FILE, "", col_);
}

Token Lexer::Identifier() {
  const int startCol = col_;
  std::string text;
  while (!IsEnd() && (IsLetter(Peek()) || isdigit(Peek()) || Peek() == '_')) {
    text += Get();
  }
  if (int kw = keyword_trie_.Match(text); kw != -1) {
    return Make(static_cast<TokenType>(kw), text, startCol);
  }
  return Make(TokenType::IDENTIFIER, text, startCol);
}

Token Lexer::Number() {
  int start_col = col_;
  std::string value;

  // Целая часть
  while (!IsEnd() && isdigit(Peek())) {
    value += Get();
  }

  // Дробная часть
  if (!IsEnd() && Peek() == '.' && isdigit(Peek(1))) {
    value += Get();  // съели '.'

    while (!IsEnd() && isdigit(Peek())) {
      value += Get();
    }
  }

  return Make(TokenType::NUMBER, value, start_col);
}

Token Lexer::StringLiteral() {
  const int startCol = col_;
  std::string text;
  Get();
  while (!IsEnd() && Peek() != '"') {
    text += Get();
  }
  Get();
  return Make(TokenType::STRING_LITERAL, text, startCol);
}

Token Lexer::Symbol() {
  const char c = Get();
  const int startCol = col_ - 1;
  switch (c) {
    case '+':
      return Make(TokenType::PLUS, "+", startCol);
    case '-':
      return Make(TokenType::MINUS, "-", startCol);
    case '*':
      return Make(TokenType::STAR, "*", startCol);
    case '/':
      return Make(TokenType::SLASH, "/", startCol);
    case '%':
      return Make(TokenType::PERCENT, "%", startCol);
    case '(':
      return Make(TokenType::LPAREN, "(", startCol);
    case ')':
      return Make(TokenType::RPAREN, ")", startCol);
    case '{':
      return Make(TokenType::LBRACE, "{", startCol);
    case '}':
      return Make(TokenType::RBRACE, "}", startCol);
    case '[':
      return Make(TokenType::LBRACKET, "[", startCol);
    case ']':
      return Make(TokenType::RBRACKET, "]", startCol);
    case ',':
      return Make(TokenType::COMMA, ",", startCol);
    case ';':
      return Make(TokenType::SEMICOLON, ";", startCol);
    case '=':
      if (Peek() == '=') {
        Get();
        return Make(TokenType::EQ, "==", startCol);
      }
      return Make(TokenType::ASSIGN, "=", startCol);
    case '!':
      if (Peek() == '=') {
        Get();
        return Make(TokenType::NEQ, "!=", startCol);
      }
      break;
    case '<':
      if (Peek() == '=') {
        Get();
        return Make(TokenType::LE, "<=", startCol);
      }
      return Make(TokenType::LT, "<", startCol);
    case '>':
      if (Peek() == '=') {
        Get();
        return Make(TokenType::GE, ">=", startCol);
      }
      return Make(TokenType::GT, ">", startCol);
    default:
      break;
  }
  return Make(TokenType::UNKNOWN, std::string(1, c), startCol);
}

Token Lexer::Make(TokenType type, const std::string &text, int start_col) const {
  return {type, text, line_, start_col};
}

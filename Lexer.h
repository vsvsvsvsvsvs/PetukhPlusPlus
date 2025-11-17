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

#endif

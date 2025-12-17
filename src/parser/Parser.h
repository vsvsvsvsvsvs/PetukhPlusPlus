#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <memory>
#include "../lexer/Token.h"
#include "AST.h"

class Parser {
public:
  Parser(const std::vector<Token> &tokens);

  std::unique_ptr<ASTNode> ParseProgram();

  const std::vector<std::string>& GetErrors() const { return errors_; }

  std::vector<std::string> errors_;   // ← добавлено

private:
  const Token &Peek() const;
  const Token &Current() const;
  void Advance();
  bool IsAtEnd() const;
  bool Match(TokenType t);
  bool Match(const std::initializer_list<TokenType> &list);
  const Token &Expect(TokenType t, const std::string &errMsg);

  void AddError(const std::string &msg);

  std::unique_ptr<ASTNode> ParseTopLevel();
  std::unique_ptr<ASTNode> ParseFunction();
  std::unique_ptr<ASTNode> ParseBlock();
  std::unique_ptr<ASTNode> ParseStatement();
  std::unique_ptr<ASTNode> ParseVarDeclList(TokenType firstTypeTok);
  std::unique_ptr<ASTNode> ParseIf();
  std::unique_ptr<ASTNode> ParseWhile();
  std::unique_ptr<ASTNode> ParseDoWhile();
  std::unique_ptr<ASTNode> ParseFor();
  std::unique_ptr<ASTNode> ParseReturn();
  std::unique_ptr<ASTNode> ParseBreak();
  std::unique_ptr<ASTNode> ParseContinue();
  std::unique_ptr<ASTNode> ParseExprStmt();

  std::unique_ptr<ASTNode> ParseExpression();
  std::unique_ptr<ASTNode> ParseAssignment();
  std::unique_ptr<ASTNode> ParseNonIdExpr();
  std::unique_ptr<ASTNode> ParseEquality();
  std::unique_ptr<ASTNode> ParseRelational();
  std::unique_ptr<ASTNode> ParseAdditive();
  std::unique_ptr<ASTNode> ParseMultiplicative();
  std::unique_ptr<ASTNode> ParseUnary();
  std::unique_ptr<ASTNode> ParsePrimary();
  std::unique_ptr<ASTNode> ParsePrimaryIdTail(std::unique_ptr<ASTNode> idNode);

  std::unique_ptr<ASTNode> MakeTypeNode(const Token &typeTok);

  std::vector<Token> tokens_;
  size_t pos_;

};

#endif // PARSER_H

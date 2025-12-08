#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "Token.h"
#include "AST.h"

// LL(1) recursive-descent parser producing unified AST (ASTNode)
class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens);

    // parse program -> returns ASTNode of kind Program
    std::unique_ptr<ASTNode> ParseProgram();

private:
    const std::vector<Token> &tokens_;
    size_t pos_ = 0;

    // token helpers
    const Token &Peek() const;
    const Token &Current() const;
    void Advance();
    bool IsAtEnd() const;
    bool Match(TokenType t);
    bool Match(const std::initializer_list<TokenType> &list);
    const Token &Expect(TokenType t, const std::string &errMsg = "");

    // grammar entry points
    std::unique_ptr<ASTNode> ParseTopLevel(); // Function | Statement
    std::unique_ptr<ASTNode> ParseFunction();

    std::unique_ptr<ASTNode> ParseStatement();
    std::unique_ptr<ASTNode> ParseBlock();

    std::unique_ptr<ASTNode> ParseVarDeclList(TokenType firstTypeTok);
    std::unique_ptr<ASTNode> ParseVarDecl(); // single VarDecl (expects IDENTIFIER already consumed or not)

    std::unique_ptr<ASTNode> ParseIf();
    std::unique_ptr<ASTNode> ParseWhile();
    std::unique_ptr<ASTNode> ParseDoWhile();
    std::unique_ptr<ASTNode> ParseFor();
    std::unique_ptr<ASTNode> ParseReturn();
    std::unique_ptr<ASTNode> ParseBreak();
    std::unique_ptr<ASTNode> ParseContinue();
    std::unique_ptr<ASTNode> ParseExprStmt();

    // expressions
    std::unique_ptr<ASTNode> ParseExpression(); // ExprComma
    std::unique_ptr<ASTNode> ParseAssignment(); // AssignmentExpr
    std::unique_ptr<ASTNode> ParseNonIdExpr();
    std::unique_ptr<ASTNode> ParsePrimary();
    std::unique_ptr<ASTNode> ParsePrimaryIdTail(std::unique_ptr<ASTNode> idNode);

    std::unique_ptr<ASTNode> ParseEquality();
    std::unique_ptr<ASTNode> ParseRelational();
    std::unique_ptr<ASTNode> ParseAdditive();
    std::unique_ptr<ASTNode> ParseMultiplicative();
    std::unique_ptr<ASTNode> ParseUnary();

    // helpers
    std::unique_ptr<ASTNode> MakeTypeNode(const Token &typeTok);
};

#endif // PARSER_H

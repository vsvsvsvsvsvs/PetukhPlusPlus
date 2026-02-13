#include "Parser.h"
#include <sstream>

// -----------------------------------------------------------------------------
//  Parser implementation with error accumulation and basic recovery
// -----------------------------------------------------------------------------

Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens), pos_(0) {}

// Helpers
const Token &Parser::Peek() const {
    if (pos_ < tokens_.size()) return tokens_[pos_];
    static Token eof{TokenType::END_OF_FILE, "", 0, 0};
    return eof;
}
const Token &Parser::Current() const { return Peek(); }

void Parser::Advance() {
    if (pos_ < tokens_.size()) ++pos_;
}

bool Parser::IsAtEnd() const {
    return Peek().type == TokenType::END_OF_FILE;
}

bool Parser::Match(TokenType t) {
    if (Peek().type == t) { Advance(); return true; }
    return false;
}

bool Parser::Match(const std::initializer_list<TokenType> &list) {
    for (auto t : list) if (Peek().type == t) { Advance(); return true; }
    return false;
}

// AddError: записывает ошибку с позицией
void Parser::AddError(const std::string &msg) {
    const Token &t = Peek();
    std::ostringstream oss;
    oss << "Line " << t.line << ", col " << t.col << ": " << msg;
    errors_.push_back(oss.str());
}

// Expect: не бросает, а добавляет ошибку и продвигается
const Token &Parser::Expect(TokenType t, const std::string &errMsg) {
    if (Peek().type == t) {
        const Token &tok = Peek();
        Advance();
        return tok;
    }
    // Ошибка: ожидается другой токен
    std::ostringstream oss;
    if (errMsg.empty()) oss << "Syntax error: expected token";
    else oss << errMsg;
    if (Peek().type != TokenType::END_OF_FILE) oss << " at '" << Peek().text << "'";
    AddError(oss.str());

    // Попытка восстановления: если текущий токен - близкий (например ; ) — вернём фиктивный токен
    // Иначе — пропустим текущий токен и вернём фиктивный.
    if (!IsAtEnd()) {
        // store current location before advancing
        int line = Peek().line;
        int col = Peek().col;
        Advance();
        static Token dummy{TokenType::UNKNOWN, "", 0, 0};
        dummy.line = line;
        dummy.col = col;
        return dummy;
    } else {
        static Token eofTok{TokenType::END_OF_FILE, "", Peek().line, Peek().col};
        return eofTok;
    }
}

// ---------------- Program ----------------
std::unique_ptr<ASTNode> Parser::ParseProgram() {
    auto root = make_node(NodeKind::Program, "Program");
    while (!IsAtEnd()) {
        if (Match(TokenType::SEMICOLON)) continue;
        root->children.push_back(ParseTopLevel());
    }
    return root;
}

std::unique_ptr<ASTNode> Parser::ParseTopLevel() {
    if (Peek().type == TokenType::KW_FN) {
        return ParseFunction();
    }
    // otherwise statement at top level (allowed by grammar)
    return ParseStatement();
}

// ---------------- Function ----------------
std::unique_ptr<ASTNode> Parser::ParseFunction() {
    Expect(TokenType::KW_FN, "expected 'fn'");

    // return type
    Token retTok;
    if (!Match({TokenType::KW_INT, TokenType::KW_CHAR, TokenType::KW_DOUBLE, TokenType::KW_STRING})) {
        AddError("expected return type after 'fn'");
        // use default int token (without consuming)
        retTok = Token{TokenType::KW_INT, "int", Peek().line, Peek().col};
    } else {
        retTok = tokens_[pos_ - 1];
    }
    auto fnNode = make_node(NodeKind::Function, "Function");

    // return type child
    fnNode->children.push_back(MakeTypeNode(retTok));

    // name
    Token nameTok = Expect(TokenType::IDENTIFIER, "expected function name");
    fnNode->text = nameTok.text; // store function name in text

    // args
    Expect(TokenType::LPAREN, "expected '(' after function name");
    if (!Match(TokenType::RPAREN)) {
        // parse argument list
        while (true) {
            if (!Match({TokenType::KW_INT, TokenType::KW_CHAR, TokenType::KW_DOUBLE, TokenType::KW_STRING})) {
                AddError("expected argument type");
                // fallback: assume int, but don't consume token
                Token argTypeTok{TokenType::KW_INT, "int", Peek().line, Peek().col};
                // try to recover by not consuming and attempt to parse name
                Token argNameTok = Expect(TokenType::IDENTIFIER, "expected argument name");
                auto argNode = make_node(NodeKind::FuncArg);
                argNode->children.push_back(MakeTypeNode(argTypeTok));
                argNode->text = argNameTok.text;
                // optional [] after arg name
                if (Match(TokenType::LBRACKET)) {
                    Expect(TokenType::RBRACKET, "expected ']'");
                    argNode->isArray = true;
                }
                fnNode->children.push_back(std::move(argNode));
            } else {
                Token argTypeTok = tokens_[pos_ - 1];
                Token argNameTok = Expect(TokenType::IDENTIFIER, "expected argument name");
                auto argNode = make_node(NodeKind::FuncArg);
                argNode->children.push_back(MakeTypeNode(argTypeTok));
                argNode->text = argNameTok.text;
                // optional [] after arg name
                if (Match(TokenType::LBRACKET)) {
                    Expect(TokenType::RBRACKET, "expected ']'");
                    argNode->isArray = true;
                }
                fnNode->children.push_back(std::move(argNode));
            }

            if (Match(TokenType::COMMA)) continue;
            Expect(TokenType::RPAREN, "expected ')' after arguments");
            break;
        }
    }

    // body (scope)
    auto body = ParseBlock();
    fnNode->children.push_back(std::move(body));
    return fnNode;
}

// ---------------- Block / Statements ----------------
std::unique_ptr<ASTNode> Parser::ParseBlock() {
    Expect(TokenType::LBRACE, "expected '{'");
    auto block = make_node(NodeKind::Block, "Block");
    while (!IsAtEnd() && Peek().type != TokenType::RBRACE) {
        if (Match(TokenType::SEMICOLON)) continue;
        block->children.push_back(ParseStatement());
    }
    Expect(TokenType::RBRACE, "expected '}'");
    return block;
}

std::unique_ptr<ASTNode> Parser::ParseStatement() {
    switch (Peek().type) {
        case TokenType::LBRACE:
            return ParseBlock();
        case TokenType::KW_IF:
            return ParseIf();
        case TokenType::KW_WHILE:
            return ParseWhile();
        case TokenType::KW_DO:
            return ParseDoWhile();
        case TokenType::KW_FOR:
            return ParseFor();
        case TokenType::KW_RETURN:
            return ParseReturn();
        case TokenType::KW_BREAK:
            return ParseBreak();
        case TokenType::KW_CONTINUE:
            return ParseContinue();
        case TokenType::KW_INT:
        case TokenType::KW_CHAR:
        case TokenType::KW_DOUBLE:
        case TokenType::KW_STRING:
            // variable declaration starts with type
            return ParseVarDeclList(Peek().type);
        default:
            return ParseExprStmt();
    }
}

std::unique_ptr<ASTNode> Parser::ParseVarDeclList(TokenType firstTypeTok) {
    // consume type
    Expect(firstTypeTok, "expected type");
    Token typeTok = tokens_[ (pos_>0) ? pos_ - 1 : 0 ];
    auto listNode = make_node(NodeKind::VarDeclList, "VarDeclList");
    // store type as first child for convenience
    listNode->children.push_back(MakeTypeNode(typeTok));

    // at least one VarDecl
    while (true) {
        Token idTok = Expect(TokenType::IDENTIFIER, "expected variable name");
        auto varNode = make_node(NodeKind::VarDecl, idTok.text);

        // VarDeclSuffix = '=' Expression ArraySuffix | ArraySuffix | ε
        if (Match(TokenType::ASSIGN)) {
            varNode->children.push_back(ParseAssignment()); // init expression
            // optionally array suffix after init
            if (Match(TokenType::LBRACKET)) {
                varNode->isArray = true;
                varNode->children.push_back(ParseExpression()); // array size expr
                Expect(TokenType::RBRACKET, "expected ']'");
            }
        } else if (Match(TokenType::LBRACKET)) {
            // ArraySuffix: '[' Expression ']'
            varNode->isArray = true;
            varNode->children.push_back(ParseExpression()); // array size
            Expect(TokenType::RBRACKET, "expected ']'");
        }
        listNode->children.push_back(std::move(varNode));
        if (Match(TokenType::COMMA)) continue;
        break;
    }
    Expect(TokenType::SEMICOLON, "expected ';' after variable list");
    return listNode;
}

std::unique_ptr<ASTNode> Parser::ParseIf() {
    Expect(TokenType::KW_IF, "expected 'if'");
    Expect(TokenType::LPAREN, "expected '(' after if");
    auto cond = ParseExpression();
    Expect(TokenType::RPAREN, "expected ')' after if condition");
    auto thenScope = ParseBlock();

    auto ifNode = make_node(NodeKind::If, "If");
    ifNode->children.push_back(std::move(cond));
    ifNode->children.push_back(std::move(thenScope));

    // ElseIfList and ElseOpt handling
    while (true) {
        if (Peek().type == TokenType::KW_ELSE) {
            // consume 'else'
            Match(TokenType::KW_ELSE);
            if (Peek().type == TokenType::KW_IF) {
                // else if
                Match(TokenType::KW_IF);
                Expect(TokenType::LPAREN, "expected '(' after else if");
                auto ec = ParseExpression();
                Expect(TokenType::RPAREN, "expected ')' after else if cond");
                auto esc = ParseBlock();
                auto elseifNode = make_node(NodeKind::ElseIf, "ElseIf");
                elseifNode->children.push_back(std::move(ec));
                elseifNode->children.push_back(std::move(esc));
                ifNode->children.push_back(std::move(elseifNode));
                // continue loop to see further else/else if
                continue;
            } else {
                // plain else
                auto elseScope = ParseBlock();
                ifNode->children.push_back(std::move(elseScope));
                break; // else is last
            }
        }
        break;
    }

    return ifNode;
}

std::unique_ptr<ASTNode> Parser::ParseWhile() {
    Expect(TokenType::KW_WHILE, "expected 'while'");
    Expect(TokenType::LPAREN, "expected '(' after while");
    auto cond = ParseExpression();
    Expect(TokenType::RPAREN, "expected ')' after while condition");
    auto body = ParseBlock();
    auto node = make_node(NodeKind::While, "While");
    node->children.push_back(std::move(cond));
    node->children.push_back(std::move(body));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseDoWhile() {
    Expect(TokenType::KW_DO, "expected 'do'");
    auto body = ParseBlock();
    Expect(TokenType::KW_WHILE, "expected 'while' after do-block");
    Expect(TokenType::LPAREN, "expected '(' after while");
    auto cond = ParseExpression();
    Expect(TokenType::RPAREN, "expected ')'");
    Expect(TokenType::SEMICOLON, "expected ';' after do-while");
    auto node = make_node(NodeKind::DoWhile, "DoWhile");
    node->children.push_back(std::move(body));
    node->children.push_back(std::move(cond));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseFor() {
    Expect(TokenType::KW_FOR, "expected 'for'");
    Expect(TokenType::LPAREN, "expected '(' after for");

    auto node = make_node(NodeKind::For, "For");

    // Ensure the for-node always has 4 children: init, cond, step, body.
    // We pre-allocate them as nullptrs.
    node->children.resize(4);

    // --- 1. Init Part ---
    if (Peek().type == TokenType::KW_INT || Peek().type == TokenType::KW_CHAR ||
        Peek().type == TokenType::KW_DOUBLE || Peek().type == TokenType::KW_STRING) {
        // Case for declaration init: for (int i = 0; ...)
        // The existing ParseVarDeclList consumes the semicolon, which is what we want here.
        node->children[0] = ParseVarDeclList(Peek().type);
        } else if (Peek().type != TokenType::SEMICOLON) {
            // Case for expression init: for (i = 0; ...)
            node->children[0] = ParseExpression();
            Expect(TokenType::SEMICOLON, "expected ';' after for-init expression");
        } else {
            // Case for empty init: for (; ...)
            Expect(TokenType::SEMICOLON, ""); // Just consume the semicolon.
        }

    // --- 2. Condition Part ---
    if (Peek().type != TokenType::SEMICOLON) {
        node->children[1] = ParseExpression();
    }
    Expect(TokenType::SEMICOLON, "expected ';' after for condition");

    // --- 3. Step/Increment Part ---
    if (Peek().type != TokenType::RPAREN) {
        node->children[2] = ParseExpression();
    }
    Expect(TokenType::RPAREN, "expected ')' after for header");

    // --- 4. Body Part ---
    node->children[3] = ParseBlock();

    return node;
}

std::unique_ptr<ASTNode> Parser::ParseReturn() {
    Expect(TokenType::KW_RETURN, "expected 'return'");
    auto node = make_node(NodeKind::Return, "Return");
    if (Peek().type != TokenType::SEMICOLON) {
        node->children.push_back(ParseExpression());
    }
    Expect(TokenType::SEMICOLON, "expected ';' after return");
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseBreak() {
    Expect(TokenType::KW_BREAK, "expected 'break'");
    Expect(TokenType::SEMICOLON, "expected ';' after break");
    return make_node(NodeKind::Break, "Break");
}

std::unique_ptr<ASTNode> Parser::ParseContinue() {
    Expect(TokenType::KW_CONTINUE, "expected 'continue'");
    Expect(TokenType::SEMICOLON, "expected ';' after continue");
    return make_node(NodeKind::Continue, "Continue");
}

std::unique_ptr<ASTNode> Parser::ParseExprStmt() {
    auto n = make_node(NodeKind::ExprStmt, "ExprStmt");
    n->children.push_back(ParseExpression());
    Expect(TokenType::SEMICOLON, "expected ';' after expression");
    return n;
}

// ---------------- Expressions ----------------

std::unique_ptr<ASTNode> Parser::ParseExpression() {
    // ExprComma = AssignmentExpr { ',' AssignmentExpr }
    auto left = ParseAssignment();
    while (Match(TokenType::COMMA)) {
        auto right = ParseAssignment();
        auto comma = make_node(NodeKind::CommaExpr, ",");
        comma->children.push_back(std::move(left));
        comma->children.push_back(std::move(right));
        left = std::move(comma);
    }
    return left;
}

// AssignmentExpr = Identifier AssignOrExpr | NonIdExpr
std::unique_ptr<ASTNode> Parser::ParseAssignment() {
    // assignment has the lowest precedence except comma,
    // so first parse normal expression
    auto lhs = ParseEquality();

    // if no '=' -> it's not assignment
    if (!Match(TokenType::ASSIGN))
        return lhs;

    // check lvalue
    if (lhs->kind != NodeKind::Identifier &&
        lhs->kind != NodeKind::Index)
    {
        AddError("left side of assignment must be variable or array element");
        // still parse RHS to continue
        auto rhs = ParseAssignment();
        auto as = make_node(NodeKind::Assign, "=");
        as->children.push_back(std::move(lhs));
        as->children.push_back(std::move(rhs));
        return as;
    }

    auto rhs = ParseAssignment(); // right associative
    auto as = make_node(NodeKind::Assign, "=");
    as->children.push_back(std::move(lhs));
    as->children.push_back(std::move(rhs));
    return as;
}

std::unique_ptr<ASTNode> Parser::ParseNonIdExpr() {
    // kept for compatibility; not used now
    return ParseEquality();
}

std::unique_ptr<ASTNode> Parser::ParseEquality() {
    auto node = ParseRelational();
    while (true) {
        if (Match(TokenType::EQ)) {
            auto rhs = ParseRelational();
            auto b = make_node(NodeKind::Binary, "==");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(rhs));
            node = std::move(b);
        } else if (Match(TokenType::NEQ)) {
            auto rhs = ParseRelational();
            auto b = make_node(NodeKind::Binary, "!=");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(rhs));
            node = std::move(b);
        } else break;
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseRelational() {
    auto node = ParseAdditive();
    while (true) {
        if (Match(TokenType::LT)) {
            auto r = ParseAdditive();
            auto b = make_node(NodeKind::Binary, "<");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else if (Match(TokenType::LE)) {
            auto r = ParseAdditive();
            auto b = make_node(NodeKind::Binary, "<=");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else if (Match(TokenType::GT)) {
            auto r = ParseAdditive();
            auto b = make_node(NodeKind::Binary, ">");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else if (Match(TokenType::GE)) {
            auto r = ParseAdditive();
            auto b = make_node(NodeKind::Binary, ">=");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else break;
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseAdditive() {
    auto node = ParseMultiplicative();
    while (true) {
        if (Match(TokenType::PLUS)) {
            auto r = ParseMultiplicative();
            auto b = make_node(NodeKind::Binary, "+");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else if (Match(TokenType::MINUS)) {
            auto r = ParseMultiplicative();
            auto b = make_node(NodeKind::Binary, "-");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else break;
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseMultiplicative() {
    auto node = ParseUnary();
    while (true) {
        if (Match(TokenType::STAR)) {
            auto r = ParseUnary();
            auto b = make_node(NodeKind::Binary, "*");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else if (Match(TokenType::SLASH)) {
            auto r = ParseUnary();
            auto b = make_node(NodeKind::Binary, "/");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else if (Match(TokenType::PERCENT)) {
            auto r = ParseUnary();
            auto b = make_node(NodeKind::Binary, "%");
            b->children.push_back(std::move(node));
            b->children.push_back(std::move(r));
            node = std::move(b);
        } else break;
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseUnary() {
    if (Match(TokenType::PLUS)) {
        auto u = make_node(NodeKind::Unary, "+");
        u->children.push_back(ParseUnary());
        return u;
    }
    if (Match(TokenType::MINUS)) {
        auto u = make_node(NodeKind::Unary, "-");
        u->children.push_back(ParseUnary());
        return u;
    }
    return ParsePrimary();
}

std::unique_ptr<ASTNode> Parser::ParsePrimary() {
    if (Match(TokenType::NUMBER)) {
        return make_node(NodeKind::Number, tokens_[pos_ - 1].text);
    }
    if (Match(TokenType::STRING_LITERAL)) {
        return make_node(NodeKind::String, tokens_[pos_ - 1].text);
    }
    if (Match(TokenType::IDENTIFIER)) {
        auto id = make_node(NodeKind::Identifier, tokens_[pos_ - 1].text);
        return ParsePrimaryIdTail(std::move(id));
    }
    if (Match(TokenType::LPAREN)) {
        auto e = ParseExpression();
        Expect(TokenType::RPAREN, "expected ')'");
        return e;
    }
    // unexpected token in expression -> report and recover by returning literal 0
    std::ostringstream oss;
    oss << "unexpected token in expression: '" << Peek().text << "'";
    AddError(oss.str());
    // skip the token and return a dummy literal to allow parsing to continue
    if (!IsAtEnd()) Advance();
    return make_node(NodeKind::Number, "0");
}

std::unique_ptr<ASTNode> Parser::ParsePrimaryIdTail(std::unique_ptr<ASTNode> idNode) {
    std::unique_ptr<ASTNode> primary = std::move(idNode);
    while (true) {
        if (Match(TokenType::LPAREN)) {
            auto call = make_node(NodeKind::Call, "Call");
            call->children.push_back(std::move(primary));
            if (!Match(TokenType::RPAREN)) {
                // first arg
                call->children.push_back(ParseExpression());
                while (Match(TokenType::COMMA)) {
                    call->children.push_back(ParseExpression());
                }
                Expect(TokenType::RPAREN, "expected ')'");
            }
            primary = std::move(call);
            continue;
        }
        if (Match(TokenType::LBRACKET)) {
            auto idx = make_node(NodeKind::Index, "Index");
            idx->children.push_back(std::move(primary));
            idx->children.push_back(ParseExpression());
            Expect(TokenType::RBRACKET, "expected ']'");
            primary = std::move(idx);
            continue;
        }
        break;
    }
    return primary;
}

// ---------------- Helpers ----------------
std::unique_ptr<ASTNode> Parser::MakeTypeNode(const Token &typeTok) {
    auto tn = make_node(NodeKind::TypeNode, typeTok.text);
    // array status will be set by surrounding code if needed
    tn->isArray = false;
    return tn;
}

#include "Parser.h"
#include <cassert>

namespace Lox {

static const Token EOF_TOKEN { TokenType::Eof, {} };

const Token& Parser::peek() const
{
    if (m_cur < m_tokens.size())
        return m_tokens[m_cur];
    return EOF_TOKEN;
}

void Parser::error(std::string msg, std::string_view span)
{
    if (!span.empty()) {
        m_errors.push_back({ std::move(msg), span });
        return;
    }
    if (peek().type() == TokenType::Eof) {
        // w/out tokens there can't be errors, so there must be at least one token
        assert(m_tokens.size() > 0);
        m_errors.push_back({ std::move(msg), m_tokens.back().text() });
    } else
        m_errors.push_back({ std::move(msg), peek().text() });
}

static std::string_view merge_texts(std::string_view start, std::string_view end)
{
    assert(start.data());
    assert(start.size() > 0);
    assert(end.data());
    assert(end.size() > 0);
    assert(start.data() + start.size() <= end.data());
    return std::string_view(start.data(), end.data() - start.data() + end.size());
}

std::shared_ptr<Expr> Parser::parse_primary()
{
    auto& token = peek();
    if (token.type() == TokenType::String) {
        advance();
        return std::make_shared<StringLiteral>(std::get<std::string>(token.value()),
                                               token.text());
    } else if (token.type() == TokenType::Number) {
        advance();
        return std::make_shared<NumberLiteral>(std::get<double>(token.value()),
                                               token.text());
    } else if (token.type() == TokenType::Identifier) {
        advance();
        return std::make_shared<Identifier>(token.text(), token.text());
    } else if (token.type() == TokenType::True ||
               token.type() == TokenType::False) {
        advance();
        return std::make_shared<BoolLiteral>(std::get<bool>(token.value()),
                                             token.text());
    } else if (token.type() == TokenType::Nil) {
        advance();
        return std::make_shared<NilLiteral>(token.text());
    } else if (token.type() == TokenType::LeftParen) {
        advance();
        if (auto expr = parse_expression()) {
            if (auto& closing = peek(); closing.type() == TokenType::RightParen) {
                advance();
                return std::make_shared<GroupExpr>(expr,
                    merge_texts(token.text(), closing.text()));
            } else
                error("'(' was never closed", token.text());
        }
        return {};
    }
    error("expected expression");
    return {};
}

std::shared_ptr<Expr> Parser::parse_unary()
{
    if (auto& token = peek(); token.type() == TokenType::Minus ||
        token.type() == TokenType::Bang) {
        advance();
        if (auto expr = parse_unary()) {
            UnaryOp op = [&token]() {
                switch (token.type()) {
                case TokenType::Minus:
                    return UnaryOp::Minus;
                case TokenType::Bang:
                    return UnaryOp::Not;
                default:
                    assert(0);
                }
            }();
            return std::make_shared<UnaryExpr>(op, expr,
                merge_texts(token.text(), expr->text()));
        }
        return {};
    }
    return parse_primary();
}

std::shared_ptr<Expr> Parser::parse_multiply()
{
    auto left = parse_unary();
    if (!left)
        return {};

    for (;;) {
        auto& token = peek();
        if (!(token.type() == TokenType::Slash ||
            token.type() == TokenType::Star))
            break;
        advance();
        auto right = parse_unary();
        if (!right)
            return {};
        BinaryOp op = [&token]() {
            switch (token.type()) {
            case TokenType::Slash:
                return BinaryOp::Divide;
            case TokenType::Star:
                return BinaryOp::Multiply;
            default:
                assert(0);
            }
        }();
        left = std::make_shared<BinaryExpr>(op, left, right,
            merge_texts(left->text(), right->text()));
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_add()
{
    auto left = parse_multiply();
    if (!left)
        return {};

    for (;;) {
        auto& token = peek();
        if (!(token.type() == TokenType::Plus ||
            token.type() == TokenType::Minus))
            break;
        advance();
        auto right = parse_multiply();
        if (!right)
            return {};
        BinaryOp op = [&token]() {
            switch (token.type()) {
            case TokenType::Plus:
                return BinaryOp::Add;
            case TokenType::Minus:
                return BinaryOp::Subtract;
            default:
                assert(0);
            }
        }();
        left = std::make_shared<BinaryExpr>(op, left, right,
            merge_texts(left->text(), right->text()));
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_compare()
{
    auto left = parse_add();
    if (!left)
        return {};

    if (auto& token = peek(); token.type() == TokenType::EqualEqual ||
        token.type() == TokenType::BangEqual ||
        token.type() == TokenType::Less ||
        token.type() == TokenType::LessEqual ||
        token.type() == TokenType::Greater ||
        token.type() == TokenType::GreaterEqual) {
        advance();
        if (auto right = parse_add()) {
            BinaryOp op = [&token]() {
                switch (token.type()) {
                case TokenType::EqualEqual:
                    return BinaryOp::Equal;
                case TokenType::BangEqual:
                    return BinaryOp::NotEqual;
                case TokenType::Less:
                    return BinaryOp::Less;
                case TokenType::LessEqual:
                    return BinaryOp::LessOrEqual;
                case TokenType::Greater:
                    return BinaryOp::Greater;
                case TokenType::GreaterEqual:
                    return BinaryOp::GreaterOrEqual;
                default:
                    assert(0);
                }
            }();
            return std::make_shared<BinaryExpr>(op, left, right,
                merge_texts(left->text(), right->text()));
        }
        return {};
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_expression()
{
    return parse_compare();
}

std::shared_ptr<Stmt> Parser::parse_var_statement()
{
    auto& var = peek();
    assert(var.type() == TokenType::Var);
    advance();

    auto& ident = peek();
    if (ident.type() != TokenType::Identifier) {
        error("expected identifier", ident.text());
        return {};
    }
    advance();

    std::shared_ptr<Expr> init;
    if (auto& token = peek(); token.type() == TokenType::Equal) {
        advance();
        init = parse_expression();
        if (!init)
            return {};
    }

    if (auto& token = peek(); token.type() == TokenType::Semicolon) {
        advance();
        return std::make_shared<VarStmt>(
            std::make_shared<Identifier>(ident.text(), ident.text()),
            init, merge_texts(var.text(), token.text()));
    } else
        error("expected ';'", token.text());
    return {};
}

std::shared_ptr<Stmt> Parser::parse_print_statement()
{
    auto& print = peek();
    assert(print.type() == TokenType::Print);
    advance();

    if (auto& token = peek(); token.type() == TokenType::Semicolon) {
        advance();
        return std::make_shared<PrintStmt>(std::shared_ptr<Expr>(),
            merge_texts(print.text(), token.text()));
    }

    auto expr = parse_expression();
    if (!expr)
        return {};

    if (auto& token = peek(); token.type() == TokenType::Semicolon) {
        advance();
        return std::make_shared<PrintStmt>(expr,
            merge_texts(print.text(), token.text()));
    } else
        error("expected ';'", token.text());
    return {};
}

std::shared_ptr<Stmt> Parser::parse_assign_statement(std::shared_ptr<Expr> place)
{
    assert(place);

    auto val = parse_expression();
    if (!val)
        return {};

    if (auto& token = peek(); token.type() == TokenType::Semicolon) {
        advance();
        return std::make_shared<AssignStmt>(place, val,
            merge_texts(place->text(), token.text()));
    } else
        error("expected ';'", token.text());
    return {};
}

std::shared_ptr<Stmt> Parser::parse_block_statement()
{
    auto& lbrace = peek();
    assert(lbrace.type() == TokenType::LeftBrace);
    advance();

    std::vector<std::shared_ptr<Stmt>> stmts;
    for (;;) {
        if (auto& token = peek(); token.type() == TokenType::RightBrace) {
            advance();
            return std::make_shared<BlockStmt>(std::move(stmts),
                merge_texts(lbrace.text(), token.text()));
        } else if (token.type() == TokenType::Eof) {
            error("'{' was never closed", lbrace.text());
            return {};
        }
        auto stmt = parse_statement();
        if (!stmt)
            return {};
        stmts.push_back(stmt);
    }
    assert(0);
}

std::shared_ptr<Stmt> Parser::parse_statement()
{
    if (auto& token = peek(); token.type() == TokenType::Var)
        return parse_var_statement();
    else if (token.type() == TokenType::Print)
        return parse_print_statement();
    else if (token.type() == TokenType::LeftBrace)
        return parse_block_statement();

    auto expr = parse_expression();
    if (!expr)
        return {};

    if (auto& token = peek(); expr->is_identifier() &&
        token.type() == TokenType::Equal) {
        advance();
        return parse_assign_statement(expr);
    } else if (token.type() == TokenType::Semicolon) {
        advance();
        return std::make_shared<ExpressionStmt>(expr,
            merge_texts(expr->text(), token.text()));
    } else
        error("expected ';'", token.text());
    return {};
}

std::shared_ptr<Program> Parser::parse()
{
    std::vector<std::shared_ptr<Stmt>> stmts;
    while (peek().type() != TokenType::Eof) {
        auto stmt = parse_statement();
        if (!stmt)
            return {};
        stmts.push_back(stmt);
    }

    std::string_view text;
    if (stmts.size() > 0) {
        text = stmts[0]->text();
        if (stmts.size() > 1)
            text = merge_texts(text, stmts.back()->text());
    }
    return std::make_shared<Program>(std::move(stmts), text);
}

}

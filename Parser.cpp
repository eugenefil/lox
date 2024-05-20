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

bool Parser::match(TokenType next, std::string_view err_msg)
{
    auto& token = peek();
    if (token.type() == next) {
        advance();
        return true;
    }
    if (!err_msg.empty())
        error(std::string(err_msg), token.text());
    return false;
}

void Parser::error(std::string msg, std::string_view span)
{
    if (!span.empty()) {
        m_errors.push_back({ std::move(msg), span });
        return;
    }

    assert(peek().type() == TokenType::Eof);
    // w/out tokens there can't be errors, so there must be at least one token
    assert(m_tokens.size() > 0);
    m_errors.push_back({ std::move(msg), m_tokens.back().text() });
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

std::shared_ptr<Identifier> Parser::parse_identifier()
{
    if (auto& token = peek(); token.type() == TokenType::Identifier) {
        advance();
        return std::make_shared<Identifier>(token.text(), token.text());
    } else
        error("expected identifier", token.text());
    return {};
}

std::shared_ptr<Expr> Parser::parse_primary()
{
    if (auto& token = peek(); token.type() == TokenType::String) {
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
    } else
        error("expected expression", token.text());
    return {};
}

std::shared_ptr<Expr> Parser::parse_call()
{
    auto expr = parse_primary();
    if (!expr)
        return {};

    while (match(TokenType::LeftParen)) {
        auto end = peek().text();
        std::vector<std::shared_ptr<Expr>> args;
        if (!match(TokenType::RightParen)) {
            do {
                auto arg = parse_expression();
                if (!arg)
                    return {};
                args.push_back(arg);
            } while (match(TokenType::Comma));

            end = peek().text();
            if (!match(TokenType::RightParen, "expected ')'"))
                return {};
        }
        expr = std::make_shared<CallExpr>(expr, std::move(args),
            merge_texts(expr->text(), end));
    }
    return expr;
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
    return parse_call();
}

std::shared_ptr<Expr> Parser::parse_multiply()
{
    auto left = parse_unary();
    if (!left)
        return {};

    for (;;) {
        auto& token = peek();
        if (!(token.type() == TokenType::Slash ||
            token.type() == TokenType::Star ||
            token.type() == TokenType::Percent))
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
            case TokenType::Percent:
                return BinaryOp::Modulo;
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

std::shared_ptr<Expr> Parser::parse_logical_and()
{
    auto left = parse_compare();
    if (!left)
        return {};

    while (match(TokenType::And)) {
        auto right = parse_compare();
        if (!right)
            return {};
        left = std::make_shared<LogicalExpr>(LogicalOp::And, left, right,
            merge_texts(left->text(), right->text()));
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_logical_or()
{
    auto left = parse_logical_and();
    if (!left)
        return {};

    while (match(TokenType::Or)) {
        auto right = parse_logical_and();
        if (!right)
            return {};
        left = std::make_shared<LogicalExpr>(LogicalOp::Or, left, right,
            merge_texts(left->text(), right->text()));
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_expression()
{
    return parse_logical_or();
}

std::pair<bool, std::string_view> Parser::finish_statement()
{
    if (auto& token = peek(); token.type() == TokenType::Semicolon) {
        advance();
        return { true, token.text() };
    } else if (token.type() == TokenType::Eof && m_implicit_semicolon)
        return { true, {} };
    else
        error("expected ';'", token.text());
    return { false, {} };
}

std::shared_ptr<Stmt> Parser::parse_var_statement()
{
    auto& var = peek();
    assert(var.type() == TokenType::Var);
    advance();

    auto ident = parse_identifier();
    if (!ident)
        return {};

    std::shared_ptr<Expr> init;
    if (match(TokenType::Equal)) {
        init = parse_expression();
        if (!init)
            return {};
    }

    if (auto [res, end] = finish_statement(); res)
        return std::make_shared<VarStmt>(ident, init,
            merge_texts(var.text(), end.size() ? end : (
                init ? init->text() : ident->text())));
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

    if (auto [res, end] = finish_statement(); res)
        return std::make_shared<PrintStmt>(expr,
            merge_texts(print.text(), end.size() ? end : expr->text()));
    return {};
}

std::shared_ptr<Stmt> Parser::parse_assign_statement(std::shared_ptr<Expr> place)
{
    assert(place);

    auto val = parse_expression();
    if (!val)
        return {};

    if (auto [res, end] = finish_statement(); res)
        return std::make_shared<AssignStmt>(place, val,
            merge_texts(place->text(), end.size() ? end : val->text()));
    return {};
}

std::shared_ptr<BlockStmt> Parser::parse_block_statement()
{
    auto& lbrace = peek();
    if (lbrace.type() != TokenType::LeftBrace) {
        error("expected '{'", lbrace.text());
        return {};
    }
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

std::shared_ptr<Stmt> Parser::parse_if_statement()
{
    auto& if_tok = peek();
    assert(if_tok.type() == TokenType::If);
    advance();

    auto test = parse_expression();
    if (!test)
        return {};

    auto then_block = parse_block_statement();
    if (!then_block)
        return {};

    std::shared_ptr<Stmt> else_block;
    if (match(TokenType::Else)) {
        if (peek().type() == TokenType::If)
            else_block = parse_if_statement();
        else
            else_block = parse_block_statement();
        if (!else_block)
            return {};
    }
    return std::make_shared<IfStmt>(test, then_block, else_block,
        merge_texts(if_tok.text(),
            else_block ? else_block->text() : then_block->text()));
}

std::shared_ptr<Stmt> Parser::parse_while_statement()
{
    auto& while_tok = peek();
    assert(while_tok.type() == TokenType::While);
    advance();

    auto test = parse_expression();
    if (!test)
        return {};

    start_loop_context();
    auto block = parse_block_statement();
    end_loop_context();
    if (!block)
        return {};

    return std::make_shared<WhileStmt>(test, block,
        merge_texts(while_tok.text(), block->text()));
}

std::shared_ptr<Stmt> Parser::parse_for_statement()
{
    auto& for_tok = peek();
    assert(for_tok.type() == TokenType::For);
    advance();

    auto ident = parse_identifier();
    if (!ident)
        return {};

    if (auto& token = peek(); token.type() != TokenType::In) {
        error("expected 'in'", token.text());
        return {};
    }
    advance();

    auto expr = parse_expression();
    if (!expr)
        return {};

    start_loop_context();
    auto block = parse_block_statement();
    end_loop_context();
    if (!block)
        return {};

    return std::make_shared<ForStmt>(ident, expr, block,
        merge_texts(for_tok.text(), block->text()));
}

std::shared_ptr<Stmt> Parser::parse_break_statement()
{
    auto& break_tok = peek();
    assert(break_tok.type() == TokenType::Break);
    advance();

    if (!is_loop_context()) {
        error("'break' outside loop", break_tok.text());
        return {};
    }

    if (auto [res, end] = finish_statement(); res)
        return std::make_shared<BreakStmt>(end.empty() ? break_tok.text() :
            merge_texts(break_tok.text(), end));
    return {};
}

std::shared_ptr<Stmt> Parser::parse_continue_statement()
{
    auto& cont_tok = peek();
    assert(cont_tok.type() == TokenType::Continue);
    advance();

    if (!is_loop_context()) {
        error("'continue' outside loop", cont_tok.text());
        return {};
    }

    if (auto [res, end] = finish_statement(); res)
        return std::make_shared<ContinueStmt>(end.empty() ? cont_tok.text() :
            merge_texts(cont_tok.text(), end));
    return {};
}

std::shared_ptr<Stmt> Parser::parse_function_declaration()
{
    auto& fn_tok = peek();
    assert(fn_tok.type() == TokenType::Fn);
    advance();

    auto name = parse_identifier();
    if (!name)
        return {};

    if (!match(TokenType::LeftParen, "expected '('"))
        return {};

    std::vector<std::shared_ptr<Identifier>> params;
    if (!match(TokenType::RightParen)) {
        do {
            auto ident = parse_identifier();
            if (!ident)
                return {};
            params.push_back(ident);
        } while (match(TokenType::Comma));

        if (!match(TokenType::RightParen, "expected ')'"))
            return {};
    }

    auto block = parse_block_statement();
    if (!block)
        return {};

    return std::make_shared<FunctionDeclaration>(name, std::move(params), block,
        merge_texts(fn_tok.text(), block->text()));
}

std::shared_ptr<Stmt> Parser::parse_statement()
{
    if (auto& token = peek(); token.type() == TokenType::Var)
        return parse_var_statement();
    else if (token.type() == TokenType::Print)
        return parse_print_statement();
    else if (token.type() == TokenType::LeftBrace)
        return parse_block_statement();
    else if (token.type() == TokenType::If)
        return parse_if_statement();
    else if (token.type() == TokenType::While)
        return parse_while_statement();
    else if (token.type() == TokenType::For)
        return parse_for_statement();
    else if (token.type() == TokenType::Break)
        return parse_break_statement();
    else if (token.type() == TokenType::Continue)
        return parse_continue_statement();
    else if (token.type() == TokenType::Fn)
        return parse_function_declaration();

    auto expr = parse_expression();
    if (!expr)
        return {};

    if (expr->is_identifier() && match(TokenType::Equal))
        return parse_assign_statement(expr);

    if (auto [res, end] = finish_statement(); res)
        return std::make_shared<ExpressionStmt>(expr,
            end.empty() ? expr->text() : merge_texts(expr->text(), end));
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

#include "Lexer.h"
#include <cassert>
#include <charconv>
#include <unordered_map>

namespace Lox {

bool Lexer::match(char next)
{
    if (peek() == next) {
        advance();
        return true;
    }
    return false;
}

std::string_view Lexer::token_text() const
{
    assert(m_end > m_start);
    return m_input.substr(m_start, m_end - m_start);
}

Span Lexer::token_span() const
{
    assert(m_end > m_start);
    return Span { m_start, m_end - m_start };
}

constexpr bool is_ascii_alpha(char ch)
{
    return ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z');
}

constexpr bool is_ascii_digit(char ch)
{
    return '0' <= ch && ch <= '9';
}

constexpr bool is_identifier_first_char(char ch)
{
    return is_ascii_alpha(ch) || ch == '_';
}

constexpr bool is_identifier_char(char ch)
{
    return is_identifier_first_char(ch) || is_ascii_digit(ch);
}

void Lexer::error(std::string_view msg, Span span)
{
    if (span == Span {})
        span = token_span();
    assert(span.len > 0);
    assert(span.pos + span.len <= m_input.size());
    m_errors.push_back({ span, msg });
}

bool Lexer::unescape(std::string& s)
{
    std::size_t last = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '\\') {
            s[last++] = s[i];
            continue;
        }
        char sub = 0;
        assert(i + 1 < s.size()); // backslash can't be last
        switch (auto ch = s[++i]) {
        case 't':
            sub = '\t';
            break;
        case 'r':
            sub = '\r';
            break;
        case 'n':
            sub = '\n';
            break;
        case '"':
        case '\\':
            sub = ch;
            break;
        case '\n':
            continue;
        default:
            // m_start + 1 -- start of string value after opening quote
            // i - 1 -- index of backslash inside string value
            error("unknown escape sequence", { (m_start + 1) + (i - 1), 2 });
            return false;
        }
        s[last++] = sub;
    }
    s.resize(last);
    return true;
}

static const std::unordered_map<std::string_view, TokenType> keywords = {
    { "and", TokenType::And },
    { "class", TokenType::Class },
    { "else", TokenType::Else },
    { "false", TokenType::False },
    { "fun", TokenType::Fun },
    { "for", TokenType::For },
    { "if", TokenType::If },
    { "nil", TokenType::Nil },
    { "or", TokenType::Or },
    { "print", TokenType::Print },
    { "return", TokenType::Return },
    { "super", TokenType::Super },
    { "this", TokenType::This },
    { "true", TokenType::True },
    { "var", TokenType::Var },
    { "while", TokenType::While },
};

std::vector<Token> Lexer::lex()
{
    std::vector<Token> tokens;

    auto add_token = [&](TokenType type,
                         Token::ValueType&& value = Token::DefaultValueType()) {
            tokens.push_back(Token {
                type,
                token_span(),
                std::move(value),
            });
            consume();
    };

    while (m_start < m_input.size()) {
        assert(m_start == m_end);
        auto ch = next();
        advance();
        switch (ch) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            consume();
            break;
        case '(':
            add_token(TokenType::LeftParen);
            break;
        case ')':
            add_token(TokenType::RightParen);
            break;
        case '{':
            add_token(TokenType::LeftBrace);
            break;
        case '}':
            add_token(TokenType::RightBrace);
            break;
        case ',':
            add_token(TokenType::Comma);
            break;
        case '.':
            add_token(TokenType::Dot);
            break;
        case '-':
            add_token(TokenType::Minus);
            break;
        case '+':
            add_token(TokenType::Plus);
            break;
        case ';':
            add_token(TokenType::Semicolon);
            break;
        case '/':
            if (match('/')) {
                while (more() && next() != '\n')
                    advance();
                add_token(TokenType::Comment);
            } else
                add_token(TokenType::Slash);
            break;
        case '*':
            add_token(TokenType::Star);
            break;
        case '!':
            add_token(match('=') ? TokenType::BangEqual : TokenType::Bang);
            break;
        case '=':
            add_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
            break;
        case '>':
            add_token(match('=') ? TokenType::GreaterEqual : TokenType::Greater);
            break;
        case '<':
            add_token(match('=') ? TokenType::LessEqual : TokenType::Less);
            break;
        case '"': {
            int num_escapes = 0;
            while (more() && next() != '"') {
                auto ch = next();
                advance();
                if (ch == '\\') {
                    ++num_escapes;
                    if (more())
                        advance();
                }
            }
            if (!more()) {
                error("unterminated string");
                add_token(TokenType::Invalid);
                break;
            }
            advance();
            assert(m_end >= m_start + 2);
            auto substr = m_input.substr(m_start + 1, m_end - m_start - 2);
            auto value = std::string(substr);
            if (num_escapes > 0 && !unescape(value)) {
                add_token(TokenType::Invalid);
                break;
            }
            add_token(TokenType::String, std::move(value));
            break;
        }
        default:
            if (is_identifier_first_char(ch)) {
                while (is_identifier_char(peek()))
                    advance();
                if (auto keyword = keywords.find(token_text());
                    keyword != keywords.end()) {
                    switch (auto type = keyword->second) {
                    case TokenType::False:
                        add_token(type, false);
                        break;
                    case TokenType::True:
                        add_token(type, true);
                        break;
                    default:
                        add_token(type);
                    }
                } else
                    add_token(TokenType::Identifier);
            } else if (is_ascii_digit(ch)) {
                double num = 0;
                auto ptr_start = m_input.data() + m_start;
                auto [ptr_end, ec] = std::from_chars(ptr_start, m_input.end(), num);
                assert(ec != std::errc::invalid_argument); // we have at least one digit
                assert(ptr_end > ptr_start);
                m_end = m_start + (ptr_end - ptr_start);
                assert(m_end <= m_input.size());
                if (ec == std::errc())
                    add_token(TokenType::Number, num);
                else if (ec == std::errc::result_out_of_range) {
                    error("literal exceeds range of double-precision floating point");
                    add_token(TokenType::Invalid);
                } else
                    assert(0); // unknown error, shouldn't happen
            } else {
                error("unknown token");
                add_token(TokenType::Invalid);
            }
        }
    }
    return tokens;
}

}

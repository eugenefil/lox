#include "Scanner.h"
#include <cassert>
#include <charconv>
#include <unordered_map>

namespace Lox {

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    out << token.text();
    return out;
}

bool Scanner::match(char next)
{
    if (peek() == next) {
        advance();
        return true;
    }
    return false;
}

std::string_view Scanner::token_text() const
{
    assert(m_end > m_beg);
    return m_input.substr(m_beg, m_end - m_beg);
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

bool Scanner::unescape(std::string& s)
{
    std::size_t next = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '\\') {
            s[next++] = s[i];
            continue;
        }
        char sub = 0;
        assert(i + 1 < s.size()); // backslash can't be last
        switch (auto ch = s[i++ + 1]) {
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
            error("unknown escape sequence");
            return false;
        }
        s[next++] = sub;
    }
    s.resize(next);
    return true;
}

static const std::unordered_map<std::string_view, Token::Type> keywords = {
    { "and", Token::Type::And },
    { "class", Token::Type::Class },
    { "else", Token::Type::Else },
    { "false", Token::Type::False },
    { "fun", Token::Type::Fun },
    { "for", Token::Type::For },
    { "if", Token::Type::If },
    { "nil", Token::Type::Nil },
    { "or", Token::Type::Or },
    { "print", Token::Type::Print },
    { "return", Token::Type::Return },
    { "super", Token::Type::Super },
    { "this", Token::Type::This },
    { "true", Token::Type::True },
    { "var", Token::Type::Var },
    { "while", Token::Type::While },
};

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;

    auto add_token = [&](Token::Type type,
                         Token::ValueType&& value = Token::DefaultValueType()) {
            tokens.push_back(Token {
                type,
                token_text(),
                std::move(value),
            });
            m_beg = m_end;
    };

    while (m_beg < m_input.size()) {
        auto ch = m_input[m_beg];
        m_end = m_beg + 1;
        switch (ch) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            m_beg = m_end;
            break;
        case '(':
            add_token(Token::Type::LeftParen);
            break;
        case ')':
            add_token(Token::Type::RightParen);
            break;
        case '{':
            add_token(Token::Type::LeftBrace);
            break;
        case '}':
            add_token(Token::Type::RightBrace);
            break;
        case ',':
            add_token(Token::Type::Comma);
            break;
        case '.':
            add_token(Token::Type::Dot);
            break;
        case '-':
            add_token(Token::Type::Minus);
            break;
        case '+':
            add_token(Token::Type::Plus);
            break;
        case ';':
            add_token(Token::Type::Semicolon);
            break;
        case '/':
            if (match('/')) {
                while (more() && next() != '\n')
                    advance();
                add_token(Token::Type::Comment);
            } else
                add_token(Token::Type::Slash);
            break;
        case '*':
            add_token(Token::Type::Star);
            break;
        case '!':
            add_token(match('=') ? Token::Type::BangEqual : Token::Type::Bang);
            break;
        case '=':
            add_token(match('=') ? Token::Type::EqualEqual : Token::Type::Equal);
            break;
        case '>':
            add_token(match('=') ? Token::Type::GreaterEqual : Token::Type::Greater);
            break;
        case '<':
            add_token(match('=') ? Token::Type::LessEqual : Token::Type::Less);
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
                add_token(Token::Type::Invalid);
                break;
            }
            advance();
            assert(m_end >= m_beg + 2);
            auto substr = m_input.substr(m_beg + 1, m_end - m_beg - 2);
            auto value = std::string(substr);
            if (num_escapes > 0 && !unescape(value)) {
                add_token(Token::Type::Invalid);
                break;
            }
            add_token(Token::Type::String, std::move(value));
            break;
        }
        default:
            if (is_identifier_first_char(ch)) {
                while (is_identifier_char(peek()))
                    advance();
                if (auto keyword = keywords.find(token_text());
                    keyword != keywords.end()) {
                    switch (auto type = keyword->second) {
                    case Token::Type::False:
                        add_token(type, false);
                        break;
                    case Token::Type::True:
                        add_token(type, true);
                        break;
                    default:
                        add_token(type);
                    }
                } else
                    add_token(Token::Type::Identifier);
            } else if (is_ascii_digit(ch)) {
                double num = 0;
                auto start = m_input.data() + m_beg;
                auto [ptr, ec] = std::from_chars(start, m_input.end(), num);
                assert(ec != std::errc::invalid_argument); // we have at least one digit
                assert(ptr > start);
                m_end = m_beg + (ptr - start);
                assert(m_end <= m_input.size());
                if (ec == std::errc())
                    add_token(Token::Type::Number, num);
                else if (ec == std::errc::result_out_of_range) {
                    error("literal exceeds range of double-precision floating point");
                    add_token(Token::Type::Invalid);
                } else
                    assert(0); // unknown error, shouldn't happen
            } else {
                error("invalid token");
                add_token(Token::Type::Invalid);
            }
        }
    }
    return tokens;
}

}

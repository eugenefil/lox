#include <variant>
#include <vector>
#include <string>

struct Token {
    enum class Type {
        LeftParen, RightParen, LeftBrace, RightBrace,
        Comma, Dot, Minus, Plus, Semicolon, Slash, Star,

        Bang, BangEqual,
        Equal, EqualEqual,
        Greater, GreaterEqual,
        Less, LessEqual,

        Identifier, String, Number,

        And, Class, Else, False, Fun, For, If, Nil, Or,
        Print, Return, Super, This, True, Var, While,
    };

    using Literal = std::variant<std::monostate, double, std::string>;

    Type m_type;
    std::string_view m_lexeme;
    Literal m_literal;

    Token(Type type, std::string_view lexeme)
        : m_type(type)
        , m_lexeme(lexeme)
    {}

    friend std::ostream& operator<<(std::ostream& out, const Token& token);
};

class Scanner {
public:
    Scanner(std::string_view source) : m_source(source)
    {}

    std::vector<Token> scan();

private:
    std::string_view m_source;
};

#include "Lexer.h"

#include <iostream>

int main()
{
    std::string source = "(+){-}";
    for (const auto& tok : Lox::Lexer(source).lex())
        std::cout << tok << '\n';
    return 0;
}

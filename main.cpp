#include "Scanner.h"

#include <iostream>

int main()
{
    std::string source = "(+){-}";
    for (const auto& tok : Lox::Scanner(source).scan())
        std::cout << tok << '\n';
    return 0;
}

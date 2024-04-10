#include "scanner.h"

#include <iostream>

int main()
{
    std::string source = "(+){-}";
    for (const auto& tok : Scanner(source).scan())
        std::cout << tok << '\n';
    return 0;
}

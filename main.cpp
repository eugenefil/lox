#include "Parser.h"
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

static std::string argv0;

[[noreturn]] void usage(bool error = false)
{
    (error ? std::cerr : std::cout) <<
    "Usage: " << argv0 << " [OPTIONS] [FILE]\n"
    "Without FILE, start repl. Otherwise, eval FILE.\n"
    "\n"
    "Options:\n"
    "  -h, --help    Display this message\n";
    std::exit(error);
}

[[noreturn]] void errusage() { usage(true); }

void repl()
{
    for (;;) {
        std::cout << ">>> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
            break;
        }
        Lox::Lexer lexer(line);
        auto tokens = lexer.lex();
        if (lexer.has_errors()) {
            continue;
        }
        Lox::Parser parser(std::move(tokens));
        auto expr = parser.parse();
        if (parser.has_errors()) {
            continue;
        }
        std::cout << expr->dump() << std::endl;
    }
}

int run(std::string_view)
{
    return 0;
}

int main(int argc, char* argv[])
{
    using namespace std::string_view_literals;
    argv0 = fs::path(argv[0]).filename();

    if (argc == 1) {
        repl();
        return 0;
    } else if (argc != 2)
        errusage();
    else if (argv[1] == "-h"sv || argv[1] == "--help"sv)
        usage();
    return run(argv[1]);
}

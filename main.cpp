#include "Parser.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <cmath>
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

void print_errors(const std::vector<Lox::Error>& errors,
                  std::string_view source, std::string_view filename)
{
    Lox::SourceMap smap(source);
    for (auto& error : errors) {
        auto range = smap.span_to_range(error.span);
        assert(range.valid());
        auto [start, end] = range;
        assert(start.line_num == end.line_num);
        auto line = smap.line(start.line_num);
        assert(line.data());
        auto num_len = static_cast<std::size_t>(std::log10(end.line_num)) + 1;
        auto spacer = std::string(num_len, ' ');
        auto marker = std::string(start.col_num - 1, ' ') +
            std::string(end.col_num - start.col_num, '^');
        std::cerr <<
            "error: " << error.msg << '\n' <<
            spacer << "--> " << filename << ':' << start.line_num <<
                ':' << start.col_num << '\n' <<
            spacer << " |" << '\n' <<
            start.line_num << " | " << line << '\n' <<
            spacer << " | " << marker << '\n' <<
            spacer << " |" << '\n' <<
            '\n';
    }
}

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
            print_errors(lexer.errors(), line, "stdin");
            continue;
        }
        Lox::Parser parser(std::move(tokens));
        auto expr = parser.parse();
        if (parser.has_errors()) {
            continue;
        }
        if (expr)
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

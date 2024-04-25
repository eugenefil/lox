#include "Parser.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <cmath>
#include <unistd.h>
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
                  std::string_view source, std::string_view filename,
                  bool color = false)
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

        constexpr auto red_bold = "\033[31;1m";
        constexpr auto blue_bold = "\033[34;1m";
        constexpr auto bold = "\033[39;1m";
        constexpr auto reset = "\033[0m";
        std::cerr << std::string()
            .append(color ? red_bold : "")
            .append("error")
            .append(color ? bold : "")
            .append(": ")
            .append(error.msg)
            .append(1, '\n')

            .append(color ? blue_bold : "")
            .append(spacer)
            .append("--> ")
            .append(color ? reset : "")
            .append(filename)
            .append(1, ':')
            .append(std::to_string(start.line_num))
            .append(1, ':')
            .append(std::to_string(start.col_num))
            .append(1, '\n')

            .append(color ? blue_bold : "")
            .append(spacer)
            .append(" |")
            .append(1, '\n')

            .append(color ? blue_bold : "")
            .append(std::to_string(start.line_num))
            .append(" | ")
            .append(color ? reset : "")
            .append(line)
            .append(1, '\n')

            .append(color ? blue_bold : "")
            .append(spacer)
            .append(" | ")
            .append(color ? red_bold : "")
            .append(marker)
            .append(color ? reset : "")
            .append("\n\n");
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
            print_errors(lexer.errors(), line, "stdin", isatty(STDERR_FILENO));
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

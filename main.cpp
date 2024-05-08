#include "Parser.h"
#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cmath>
#include <cstdio>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string argv0;

[[noreturn]] static void usage(bool error = false)
{
    (error ? std::cerr : std::cout) <<
    "Usage: " << argv0 << " [OPTIONS] [FILE]\n"
    "Without FILE, start repl. Otherwise, eval FILE.\n"
    "\n"
    "Options:\n"
    "  -h, --help    Display this message\n";
    std::exit(error);
}

[[noreturn]] static void errusage() { usage(true); }

static void print_errors(const std::vector<Lox::Error>& errors,
                         std::string_view source, std::string_view filename,
                         bool color = false)
{
    Lox::SourceMap smap(source);
    for (auto& error : errors) {
        auto [start, end] = smap.span_to_range(error.span);
        assert(start.line_num == end.line_num);
        auto line = smap.line(start.line_num);
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

static bool eval(std::string_view source, std::string_view path,
                 Lox::Interpreter& interp, bool repl_mode)
{
    Lox::Lexer lexer(source);
    auto tokens = lexer.lex();
    if (lexer.has_errors()) {
        print_errors(lexer.errors(), source, path, isatty(STDERR_FILENO));
        return false;
    }

    Lox::Parser parser(std::move(tokens));
    parser.repl_mode(repl_mode);
    auto program = parser.parse();
    if (parser.has_errors()) {
        print_errors(parser.errors(), source, path, isatty(STDERR_FILENO));
        return false;
    }

    interp.interpret(program);
    if (interp.has_errors()) {
        print_errors(interp.errors(), source, path, isatty(STDERR_FILENO));
        return false;
    }

    return true;
}

static void repl()
{
    Lox::Interpreter interp;
    interp.repl_mode(true);
    for (;;) {
        std::cerr << ">>> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cerr << '\n';
            break;
        }
        eval(line, "<stdin>", interp, true);
    }
}

static int run(std::string path)
{
    std::ostringstream buf;
    if (path == "-") {
        path = "<stdin>";
        while (buf << std::cin.rdbuf())
            ;
        if (buf.bad()) {
            std::perror("error: cannot read from stdin");
            return 1;
        }
    } else {
        std::ifstream fin(path);
        if (!fin.is_open()) {
            std::perror(("error: cannot open '" + path + "'").c_str());
            return 1;
        }
        while (buf << fin.rdbuf())
            ;
        if (buf.bad()) {
            std::perror(("error: cannot read from '" + path + "'").c_str());
            return 1;
        }
        fin.close();
    }

    Lox::Interpreter interp;
    if (eval(buf.view(), path, interp, false))
        return 0;
    return 1;
}

int main(int argc, char* argv[])
{
    using namespace std::string_view_literals;
    argv0 = fs::path(argv[0]).filename();

    if (argc == 1) {
        if (isatty(STDIN_FILENO)) {
            repl();
            return 0;
        } else
            return run("-");
    } else if (argc != 2)
        errusage();
    else if (argv[1] == "-h"sv || argv[1] == "--help"sv)
        usage();
    return run(argv[1]);
}

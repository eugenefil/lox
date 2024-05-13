#include "Parser.h"
#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace fs = std::filesystem;

static std::string argv0;

[[noreturn]] static void usage(bool error = false)
{
    (error ? std::cerr : std::cout) <<
    "Usage: " << argv0 << " [OPTIONS] [FILE]\n"
    "If given, eval FILE. Otherwise, eval stdin. If stdin is a tty, start REPL instead.\n"
    "\n"
    "Options:\n"
    "  -h, --help    Display this message\n";
    std::exit(error);
}

class Formatter {
public:
    Formatter(bool has_color) : m_has_color(has_color)
    {}

    std::string colorize(std::string_view color, std::string_view text)
    {
        return std::string(m_has_color ? color : "")
            .append(text)
            .append(m_has_color ? "\033[0m" : "");
    }

    std::string red(std::string_view text) { return colorize("\033[31;1m", text); }
    std::string blue(std::string_view text) { return colorize("\033[34;1m", text); }
    std::string bold(std::string_view text) { return colorize("\033[39;1m", text); }

    std::string error(std::string_view text)
    {
        return red("error")
            .append(bold(": "))
            .append(bold(text))
            .append("\n");
    }

    std::string strerror(std::string_view text)
    {
        return error(std::string(text) + ": " + std::strerror(errno));
    }

private:
    bool m_has_color { false };
};

[[noreturn]] static void errusage() { usage(true); }

static void print_errors(const std::vector<Lox::Error>& errors,
                         std::string_view source, std::string_view filename)
{
    Lox::SourceMap smap(source);
    Formatter fmt(isatty(STDERR_FILENO));
    for (auto& error : errors) {
        auto [start, end] = smap.span_to_range(error.span);
        assert(start.line_num == end.line_num);
        auto num_len = static_cast<std::size_t>(std::log10(end.line_num)) + 1;
        auto spacer = std::string(num_len, ' ');

        // convert tabs to spaces
        auto orig_line = smap.line(start.line_num);
        std::string line;
        constexpr std::size_t spaces_in_tab = 4;
        const std::string spaces(spaces_in_tab, ' ');
        auto start_col = start.col_num;
        auto end_col = end.col_num;
        for (std::size_t i = 0; i < orig_line.size(); ++i) {
            auto ch = orig_line[i];
            if (ch == '\t') {
                line += spaces;
                if (i + 1 < start.col_num)
                    start_col += spaces_in_tab - 1;
                if (i + 1 < end.col_num)
                    end_col += spaces_in_tab - 1;
            } else
                line += ch;
        }
        assert(end_col > start_col);
        auto marker = std::string(start_col - 1, ' ') +
            std::string(end_col - start_col, '^');

        std::cerr <<
            // error message line
            fmt.error(error.msg)
            // source location line
            .append(spacer)
            .append(fmt.blue("--> "))
            .append(filename)
            .append(":")
            .append(std::to_string(start.line_num))
            .append(":")
            .append(std::to_string(start.col_num))
            .append("\n")
            // padding line
            .append(spacer)
            .append(fmt.blue(" |"))
            .append("\n")
            // source line
            .append(fmt.blue(std::to_string(start.line_num) + " | "))
            .append(line)
            .append("\n")
            // marker line
            .append(spacer)
            .append(fmt.blue(" | "))
            .append(fmt.red(marker))
            .append("\n");
    }
}

static bool eval(std::string_view source, std::string_view path,
                 Lox::Interpreter& interp, bool repl_mode)
{
    Lox::Lexer lexer(source);
    auto tokens = lexer.lex();
    if (lexer.has_errors()) {
        print_errors(lexer.errors(), source, path);
        return false;
    }

    Lox::Parser parser(std::move(tokens));
    parser.repl_mode(repl_mode);
    auto program = parser.parse();
    if (parser.has_errors()) {
        print_errors(parser.errors(), source, path);
        return false;
    }

    interp.interpret(program);
    if (interp.has_errors()) {
        print_errors(interp.errors(), source, path);
        return false;
    }

    return true;
}

static void repl()
{
    Lox::Interpreter interp;
    interp.repl_mode(true);
    rl_outstream = stderr;
    for (char* line = NULL; (line = readline(">>> ")); free(line)) {
        if (*line) {
            eval(line, "<stdin>", interp, true);
            add_history(line);
        }
    }
}

static int run(std::string path)
{
    std::ostringstream buf;
    Formatter fmt(isatty(STDERR_FILENO));
    if (path == "-") {
        path = "<stdin>";
        while (buf << std::cin.rdbuf())
            ;
        if (buf.bad()) {
            std::cerr << fmt.strerror("cannot read from '" + path + "'");
            return 1;
        }
    } else {
        std::ifstream fin(path);
        if (!fin.is_open()) {
            std::cerr << fmt.strerror("cannot open '" + path + "'");
            return 1;
        }
        while (buf << fin.rdbuf())
            ;
        if (buf.bad()) {
            std::cerr << fmt.strerror("cannot read from '" + path + "'");
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

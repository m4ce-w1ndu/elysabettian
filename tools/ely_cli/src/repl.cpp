#include <ely/cli/repl.hpp>
#include <ely/cli/runner.hpp>
#include <ely/compiler/scanner.hpp>
#include <ely/compiler/token.hpp>

#include <replxx.hxx>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ely;

const std::vector<std::string> kKeywords = {
    "var", "func", "return", "if", "else", "while", "for",
    "print", "import", "true", "false", "null", "class", "this", "super"
};

const std::vector<std::string> kBuiltinModules = {
    "math", "system", "io"
};

const std::vector<std::string> kBuiltinFunctions = {
    "clock", "read", "sleep", "flush", "sin", "cos", "tan",
    "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh",
    "sqrt", "cbrt", "exp", "ln", "log10", "log2", "pow",
    "floor", "ceil", "round", "trunc", "abs"
};

[[nodiscard]] std::string get_history_file_path() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = std::getenv("USERPROFILE");
    }
    if (home) {
        return (std::filesystem::path(home) / ".ely_history").string();
    }
    return ".ely_history";
}

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept {
    const auto start = sv.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = sv.find_last_not_of(" \t\n\r");
    return sv.substr(start, end - start + 1);
}

/**
 * Checks whether the source code has unclosed blocks or parentheses.
 * Returns true if more input is required (multi-line statement).
 */
[[nodiscard]] bool is_incomplete(std::string_view source) noexcept {
    compiler::Scanner scanner(source);
    int32_t paren_depth = 0;
    int32_t curly_depth = 0;
    int32_t square_depth = 0;

    while (true) {
        const auto token = scanner.scan_token();
        if (token.token_type == compiler::TokenType::Eof) {
            break;
        }

        switch (token.token_type) {
            case compiler::TokenType::OpenParen:
                ++paren_depth;
                break;
            case compiler::TokenType::CloseParen:
                --paren_depth;
                break;
            case compiler::TokenType::OpenCurly:
                ++curly_depth;
                break;
            case compiler::TokenType::CloseCurly:
                --curly_depth;
                break;
            case compiler::TokenType::OpenSquare:
                ++square_depth;
                break;
            case compiler::TokenType::CloseSquare:
                --square_depth;
                break;
            default:
                break;
        }
    }

    return paren_depth > 0 || curly_depth > 0 || square_depth > 0;
}

replxx::Replxx::completions_t hook_completion(const std::string& input, int& context_len) {
    replxx::Replxx::completions_t completions;

    // Find the current identifier word prefix being typed
    size_t start = input.length();
    while (start > 0) {
        const char ch = input[start - 1];
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
            --start;
        } else {
            break;
        }
    }
    const std::string prefix = input.substr(start);
    context_len = static_cast<int>(prefix.length());

    if (prefix.empty()) {
        return completions;
    }

    auto check_matches = [&](const std::vector<std::string>& candidates) {
        for (const auto& word : candidates) {
            if (word.rfind(prefix, 0) == 0) {
                completions.emplace_back(word);
            }
        }
    };

    check_matches(kKeywords);
    check_matches(kBuiltinModules);
    check_matches(kBuiltinFunctions);

    return completions;
}

replxx::Replxx::hints_t hook_hint(const std::string& input, int& context_len, replxx::Replxx::Color& color) {
    replxx::Replxx::hints_t hints;

    size_t start = input.length();
    while (start > 0) {
        const char ch = input[start - 1];
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
            --start;
        } else {
            break;
        }
    }
    const std::string prefix = input.substr(start);
    context_len = static_cast<int>(prefix.length());

    if (prefix.empty()) {
        return hints;
    }

    auto check_hints = [&](const std::vector<std::string>& candidates) {
        for (const auto& word : candidates) {
            if (word.rfind(prefix, 0) == 0 && word != prefix) {
                hints.emplace_back(word);
            }
        }
    };

    check_hints(kKeywords);
    check_hints(kBuiltinModules);
    check_hints(kBuiltinFunctions);

    color = replxx::Replxx::Color::GRAY;
    return hints;
}

void hook_highlighter(const std::string& input, replxx::Replxx::colors_t& colors) {
    colors.assign(input.length(), replxx::Replxx::Color::DEFAULT);

    compiler::Scanner scanner(input);
    const char* base = input.data();

    while (true) {
        const auto token = scanner.scan_token();
        if (token.token_type == compiler::TokenType::Eof) {
            break;
        }

        const auto offset = static_cast<size_t>(token.lexeme.data() - base);
        const auto len = token.lexeme.length();
        if (offset + len > colors.size()) {
            break;
        }

        replxx::Replxx::Color col = replxx::Replxx::Color::DEFAULT;

        switch (token.token_type) {
            case compiler::TokenType::Var:
            case compiler::TokenType::Func:
            case compiler::TokenType::Return:
            case compiler::TokenType::If:
            case compiler::TokenType::Else:
            case compiler::TokenType::While:
            case compiler::TokenType::For:
            case compiler::TokenType::Print:
            case compiler::TokenType::Import:
            case compiler::TokenType::Class:
            case compiler::TokenType::This:
            case compiler::TokenType::Super:
                col = replxx::Replxx::Color::BRIGHTMAGENTA;
                break;

            case compiler::TokenType::True:
            case compiler::TokenType::False:
            case compiler::TokenType::Null:
                col = replxx::Replxx::Color::YELLOW;
                break;

            case compiler::TokenType::Number:
                col = replxx::Replxx::Color::BRIGHTCYAN;
                break;

            case compiler::TokenType::String:
                col = replxx::Replxx::Color::BRIGHTGREEN;
                break;

            case compiler::TokenType::Plus:
            case compiler::TokenType::Minus:
            case compiler::TokenType::Divide:
            case compiler::TokenType::Multiply:
            case compiler::TokenType::Assign:
            case compiler::TokenType::Equality:
            case compiler::TokenType::NonEquality:
            case compiler::TokenType::LessThan:
            case compiler::TokenType::LessThanEqual:
            case compiler::TokenType::GreaterThan:
            case compiler::TokenType::GreaterThanEqual:
            case compiler::TokenType::And:
            case compiler::TokenType::Or:
            case compiler::TokenType::Bang:
            case compiler::TokenType::BwNot:
            case compiler::TokenType::BwAnd:
            case compiler::TokenType::BwOr:
            case compiler::TokenType::BwXor:
            case compiler::TokenType::PlusPlus:
            case compiler::TokenType::MinusMinus:
            case compiler::TokenType::PlusAssign:
            case compiler::TokenType::MinusAssign:
            case compiler::TokenType::MultiplyAssign:
            case compiler::TokenType::DivideAssign:
                col = replxx::Replxx::Color::BRIGHTRED;
                break;

            default:
                break;
        }

        if (col != replxx::Replxx::Color::DEFAULT) {
            for (size_t i = 0; i < len; ++i) {
                colors[offset + i] = col;
            }
        }
    }
}

} // namespace

namespace ely::cli {

void run_repl(vm::VM& vm, bool jit_only, bool dump_ir, bool dump_asm) {
    const std::string_view mode = jit_only ? "JIT Native Mode" : "VM Tier-up Mode";
    std::cout << "Elysabettian REPL (v0.1.0) [" << mode << "]\n";
    std::cout << "Type 'exit' or press Ctrl+D to exit.\n\n";

    replxx::Replxx rx;
    rx.install_window_change_handler();

    rx.set_completion_callback([](const std::string& input, int& context_len) {
        return hook_completion(input, context_len);
    });

    rx.set_hint_callback([](const std::string& input, int& context_len, replxx::Replxx::Color& color) {
        return hook_hint(input, context_len, color);
    });

    rx.set_highlighter_callback([](const std::string& input, replxx::Replxx::colors_t& colors) {
        hook_highlighter(input, colors);
    });

    rx.set_max_history_size(1000);
    rx.set_max_hint_rows(3);
    rx.set_word_break_characters(" \t\n\r+-*/=<>!&|^~();,{}[].");

    const std::string history_path = get_history_file_path();
    rx.history_load(history_path);

    const char* prompt_normal = "\x1b[1;32mely>\x1b[0m ";
    const char* prompt_multiline = "\x1b[1;34m... \x1b[0m ";

    std::string accumulator;

    while (true) {
        const char* prompt = accumulator.empty() ? prompt_normal : prompt_multiline;
        const char* input = rx.input(prompt);

        if (!input) {
            // EOF (Ctrl+D)
            std::cout << '\n';
            break;
        }

        const std::string current_line(input);
        const auto trimmed = trim(current_line);

        // Allow exiting from top-level prompt
        if (accumulator.empty() && (trimmed == "exit" || trimmed == "quit")) {
            break;
        }

        if (accumulator.empty() && trimmed.empty()) {
            continue;
        }

        // Python-like multi-line termination: double Enter on empty line finishes a multi-line block
        if (!accumulator.empty() && trimmed.empty() && !is_incomplete(accumulator)) {
            rx.history_add(accumulator);
            rx.history_save(history_path);

            static_cast<void>(run_source(accumulator, vm, jit_only, dump_ir, dump_asm));
            accumulator.clear();
            continue;
        }

        if (!accumulator.empty()) {
            accumulator += '\n';
        }
        accumulator += current_line;

        // If the statement is complete (all braces/parens matched), execute it!
        if (!is_incomplete(accumulator)) {
            rx.history_add(accumulator);
            rx.history_save(history_path);

            static_cast<void>(run_source(accumulator, vm, jit_only, dump_ir, dump_asm));
            accumulator.clear();
        }
    }

    rx.history_save(history_path);
}

} // namespace ely::cli

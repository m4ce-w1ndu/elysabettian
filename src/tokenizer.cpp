#include "tokenizer.hpp"

#include <cctype>

token_t tokenizer_t::scan_token()
{
    skip_whitespace();
    
    start = current;
    
    if (is_at_end()) return make_token(token_type_t::_EOF);
    
    auto c = advance();
    
    if (isdigit(c)) return number();
    if (isalpha(c)) return identifier();
    
    switch (c) {
        case '(': return make_token(token_type_t::OPEN_PAREN);
        case ')': return make_token(token_type_t::CLOSE_PAREN);
        case '{': return make_token(token_type_t::OPEN_CURLY);
        case '}': return make_token(token_type_t::CLOSE_CURLY);
        case ';': return make_token(token_type_t::SEMICOLON);
        case ',': return make_token(token_type_t::COMMA);
        case '.': return make_token(token_type_t::DOT);
        case '-': return make_token(token_type_t::MINUS);
        case '+': return make_token(token_type_t::PLUS);
        case '/': return make_token(token_type_t::SLASH);
        case '*': return make_token(token_type_t::STAR);
        case '^': return make_token(token_type_t::BW_XOR);
        case '&': return make_token(match('&') ? token_type_t::AND : token_type_t::BW_AND);
        case '|': return make_token(match('|') ? token_type_t::OR : token_type_t::BW_OR);
        case '!':
            return make_token(match('=') ? token_type_t::EXCL_EQUAL : token_type_t::EXCL);
        case '=':
            return make_token(match('=') ? token_type_t::EQUAL_EQUAL : token_type_t::EQUAL);
        case '<':
            return make_token(match('=') ? token_type_t::LESS_EQUAL : token_type_t::LESS);
        case '>':
            return make_token(match('=') ?
                             token_type_t::GREATER_EQUAL : token_type_t::GREATER);
            
        case '"': return string_();
        case '\'': return string_('\'');
        default: return error_token("Unexpected character.");
    }
}

bool tokenizer_t::is_at_end() const
{
    return current == source.length();
}

char tokenizer_t::advance()
{
    current++;
    return source[current - 1];
}

char tokenizer_t::peek()
{
    if (is_at_end()) return '\0';
    return source[current];
}

char tokenizer_t::peek_next()
{
    if ((current + 1) >= source.length()) return '\0';
    return source[current + 1];
}

bool tokenizer_t::match(char expected)
{
    if (is_at_end()) return false;
    if (source[current] != expected) return false;
    
    current++;
    return true;
}

token_t tokenizer_t::make_token(token_type_t type)
{
    auto text = std::string_view(&source[start], current - start);
    return token_t(type, text, line);
}

token_t tokenizer_t::error_token(const std::string& message) const
{
    return token_t(token_type_t::ERROR, message, line);
}

void tokenizer_t::skip_whitespace()
{
    while (true) {
        auto c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
                
            case '\n':
                line++;
                advance();
                break;
                
            case '/':
                if (peek_next() != '/') return;
                while (peek() != '\n' && !is_at_end()) advance();
                break;
                
            default:
                return;
        }
    }
}

token_type_t tokenizer_t::check_keyword(size_t pos, size_t len, const std::string_view& rest, token_type_t type) const
{
    if (current - start == pos + len && source.compare(start + pos, len, rest) == 0) {
        return type;
    }
    
    return token_type_t::IDENTIFIER;
}

token_type_t tokenizer_t::identifier_type()
{
    switch (source[start]) {
        case 'c': return check_keyword(1, 4, "lass", token_type_t::CLASS);
        case 'e': return check_keyword(1, 3, "lse", token_type_t::ELSE);
        case 'f':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'a': return check_keyword(2, 3, "lse", token_type_t::FALSE);
                    case 'o': return check_keyword(2, 1, "r", token_type_t::FOR);
                    case 'u': return check_keyword(2, 2, "nc", token_type_t::FUN);
                    default: return token_type_t::IDENTIFIER;
                }
            }
            break;
        case 'i': return check_keyword(1, 1, "f", token_type_t::IF);
        case 'n': return check_keyword(1, 3, "ull", token_type_t::NULLVAL);
        case 'p': return check_keyword(1, 4, "rint", token_type_t::PRINT);
        case 'r': return check_keyword(1, 5, "eturn", token_type_t::RETURN);
        case 's': return check_keyword(1, 4, "uper", token_type_t::SUPER);
        case 't':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'h': return check_keyword(2, 2, "is", token_type_t::THIS);
                    case 'r': return check_keyword(2, 2, "ue", token_type_t::TRUE);
                    default: return token_type_t::IDENTIFIER;
                }
            }
            break;
        case 'v': return check_keyword(1, 2, "ar", token_type_t::VAR);
        case 'w': return check_keyword(1, 4, "hile", token_type_t::WHILE);
        default: return token_type_t::IDENTIFIER;
    }

    return token_type_t::IDENTIFIER;
}

token_t tokenizer_t::identifier()
{
    while (isalnum(peek())) advance();
    
    return make_token(identifier_type());
}

token_t tokenizer_t::number()
{
    while (isdigit(peek())) advance();
    
    if (peek() == '.' && isdigit(peek_next())) {
        advance();
        while (isdigit(peek())) advance();
    }
    
    return make_token(token_type_t::NUMBER);
}

token_t tokenizer_t::string_(const char open_char)
{
    while (peek() != open_char && !is_at_end()) {
        if (peek() == '\n') line++;
        advance();
    }
    
    if (is_at_end()) return error_token("Unterminated string.");
    
    // The closing ".
    advance();
    return make_token(token_type_t::STRING);
}

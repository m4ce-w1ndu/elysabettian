#include "language/tokenizer.h"

#include <cctype>

Token Tokenizer::scan_token()
{
    skip_whitespace();
    
    start = current;
    
    if (is_at_end()) return make_token(TokenType::Eof);
    
    auto c = advance();
    
    if (isdigit(c)) return number();
    if (isalpha(c)) return identifier();
    
    switch (c) {
        case '(': return make_token(TokenType::OpenParen);
        case ')': return make_token(TokenType::CloseParen);
        case '[': return make_token(TokenType::OpenSquare);
        case ']': return make_token(TokenType::CloseSquare);
        case '{': return make_token(TokenType::OpenCurly);
        case '}': return make_token(TokenType::CloseCurly);
        case ';': return make_token(TokenType::Semicolon);
        case ',': return make_token(TokenType::Comma);
        case '.': return make_token(TokenType::Dot);
        case '-': return make_token(TokenType::Minus);
        case '+': return make_token(TokenType::Plus);
        case '/': return make_token(TokenType::Slash);
        case '*': return make_token(TokenType::Star);
        case '^': return make_token(TokenType::BwXor);
        case '&': return make_token(match('&') ? TokenType::And : TokenType::BwAnd);
        case '|': return make_token(match('|') ? TokenType::Or : TokenType::BwOr);
        case '!':
            return make_token(match('=') ? TokenType::ExclEqual : TokenType::Excl);
        case '=':
            return make_token(match('=') ? TokenType::EqualEqual : TokenType::Equal);
        case '<':
            return make_token(match('=') ? TokenType::LessEqual : match('<') ?
                              TokenType::LessLess : TokenType::Less);
        case '>':
            return make_token(match('=') ? TokenType::GreaterEqual : match('>') ?
                              TokenType::GreaterGreater : TokenType::Greater);
            
        case '"': return string_();
        case '\'': return string_('\'');
        default: return error_token("Unexpected character.");
    }
}

bool Tokenizer::is_at_end() const
{
    return current == source.length();
}

char Tokenizer::advance()
{
    current++;
    return source[current - 1];
}

char Tokenizer::peek()
{
    if (is_at_end()) return '\0';
    return source[current];
}

char Tokenizer::peek_next()
{
    if ((current + 1) >= source.length()) return '\0';
    return source[current + 1];
}

bool Tokenizer::match(char expected)
{
    if (is_at_end()) return false;
    if (source[current] != expected) return false;
    
    current++;
    return true;
}

Token Tokenizer::make_token(TokenType type)
{
    auto text = std::string_view(&source[start], current - start);
    return Token(type, text, line);
}

Token Tokenizer::error_token(const std::string& message) const
{
    return Token(TokenType::Error, message, line);
}

void Tokenizer::skip_whitespace()
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

TokenType Tokenizer::check_keyword(size_t pos, size_t len, const std::string_view& rest, TokenType type) const
{
    if (current - start == pos + len && source.compare(start + pos, len, rest) == 0) {
        return type;
    }
    
    return TokenType::Identifier;
}

TokenType Tokenizer::identifier_type()
{
    switch (source[start]) {
        case 'c': return check_keyword(1, 4, "lass", TokenType::Class);
        case 'e': return check_keyword(1, 3, "lse", TokenType::Else);
        case 'f':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'a': return check_keyword(2, 3, "lse", TokenType::False);
                    case 'o': return check_keyword(2, 1, "r", TokenType::For);
                    case 'u': return check_keyword(2, 2, "nc", TokenType::Func);
                    default: return TokenType::Identifier;
                }
            }
            break;
        case 'i': return check_keyword(1, 1, "f", TokenType::If);
        case 'n': return check_keyword(1, 3, "ull", TokenType::Null);
        case 'p': return check_keyword(1, 4, "rint", TokenType::Print);
        case 'r': return check_keyword(1, 5, "eturn", TokenType::Return);
        case 's': return check_keyword(1, 4, "uper", TokenType::Super);
        case 't':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'h': return check_keyword(2, 2, "is", TokenType::This);
                    case 'r': return check_keyword(2, 2, "ue", TokenType::True);
                    default: return TokenType::Identifier;
                }
            }
            break;
        case 'v': return check_keyword(1, 2, "ar", TokenType::Var);
        case 'w': return check_keyword(1, 4, "hile", TokenType::While);
        default: return TokenType::Identifier;
    }

    return TokenType::Identifier;
}

Token Tokenizer::identifier()
{
    while (isalnum(peek())) advance();
    
    return make_token(identifier_type());
}

Token Tokenizer::number()
{
    while (isdigit(peek())) advance();
    
    if (peek() == '.' && isdigit(peek_next())) {
        advance();
        while (isdigit(peek())) advance();
    }
    
    return make_token(TokenType::Number);
}

Token Tokenizer::string_(const char open_char)
{
    while (peek() != open_char && !is_at_end()) {
        if (peek() == '\n') line++;
        advance();
    }
    
    if (is_at_end()) return error_token("Unterminated string.");
    
    // The closing ".
    advance();
    return make_token(TokenType::String);
}

use super::token::{Token, TokenType};


/// Used to tokenize (transform into single tokens) source code
/// in the Elysabettian programming language
pub struct Tokenizer {
    source: String,
    start: usize,
    current: usize,
    line: i32
}

impl Tokenizer {
    /// Create a new Tokenizer for this block of code
    /// 
    /// Arguments:
    /// * `source`: reference to source code string
    pub fn new(source: &String) -> Tokenizer {
        Tokenizer {
            source: source.clone(),
            start: 0_usize,
            current: 0_usize,
            line: 1_i32
        }
    }

    pub fn get_token(&mut self) -> Token {
        // Skip all whitespaces
        self.skip_whitespaces();

        self.start = self.current;

        // Check for EOF
        if self.is_at_end() {
            return self.make_token(TokenType::Eof)
        }

        // Get the next character and identify it
        let c = self.advance();
        if c.is_numeric() { return self.number() }
        if c.is_alphanumeric() { return self.identifier() }

        match c {
            '(' => self.make_token(TokenType::OpenParen),
            ')' => self.make_token(TokenType::CloseParen),
            '[' => self.make_token(TokenType::OpenSquare),
            ']' => self.make_token(TokenType::CloseSquare),
            '{' => self.make_token(TokenType::OpenCurly),
            '}' => self.make_token(TokenType::CloseCurly),
            ';' => self.make_token(TokenType::Semicolon),
            ',' => self.make_token(TokenType::Comma),
            '.' => self.make_token(TokenType::Dot),
            '-' => self.make_token(TokenType::Minus),
            '+' => self.make_token(TokenType::Plus),
            '/' => self.make_token(TokenType::Slash),
            '*' => self.make_token(TokenType::Star),
            '^' => self.make_token(TokenType::BwXor),
            '&' => self.make_token(if self.match_token('&') { TokenType::And} else { TokenType::BwAnd}),
            '|' => self.make_token(if self.match_token('|') { TokenType::Or } else { TokenType::BwOr }),
            '!' => self.make_token(if self.match_token('=') { TokenType::ExclEqual } else { TokenType::Excl }),
            '<' => self.make_token(
                if self.match_token('=') 
                { 
                    TokenType::LessEqual
                } else if self.match_token('<'){ 
                    TokenType::LessLess
                })),
            '>' => self.make_token(if self.match_token('=') { TokenType::GreaterEqual } else {})
        }
    }

    /// Checks if the tokenizer has reached the end of the source code
    fn is_at_end(&self) -> bool {
        self.current == self.source.len()
    }

    /// Advances the tokenizer by one character
    fn advance(&mut self) -> char {
        self.current += 1;
        self.source.chars().nth(self.current - 1).unwrap()
    }

    /// Returns the character in the current position
    fn peek(&self) -> char {
        if self.is_at_end() {
            return '\0'
        }

        self.source.chars().nth(self.current).unwrap()
    }

    /// Returns the character in the next position
    fn peek_next(&self) -> char {
        if self.current + 1 >= self.source.len() {
            return '\0'
        }

        self.source.chars().nth(self.current + 1).unwrap()
    }

    /// Matches the token with the current character and advances by one position
    fn match_token(&mut self, expected: char) -> bool {
        if self.is_at_end() {
            return false
        }

        if self.source.chars().nth(self.current).unwrap() != expected {
            return false
        }

        self.current += 1;
        true
    }

    /// Creates a new token with chars at current position and given type
    fn make_token(&self, token_type: TokenType) -> Token {
        let text: String = self.source.chars()
            .skip(self.start)
            .take(self.current - self.start)
            .collect();

        Token::new(token_type, &text, self.line)
    }

    /// Creates a new Token with an error message and the corresponding line
    fn error_token(&self, message: &str) -> Token {
        Token::new(TokenType::Error, &message.to_owned(), self.line)
    }

    /// Matches all whitespaces and skips them if necessary
    fn skip_whitespaces(&mut self) {
        loop {
            // Take the current character
            let c = self.peek();

            // Match it against all whitespaces and skip them if necessary
            match c {
                ' ' | '\r' | '\t' => _ = self.advance(),
                '\n' => { self.line += 1; _= self.advance() },
                '/' => {
                    if self.peek_next() != '/' {
                        return;
                    }

                    while self.peek() != '\n' && !self.is_at_end() {
                        _ = self.advance();
                    }
                },
                _ => return
            }
        }
    }

    /// Checks a keyword and, if it is a language keyword, it sets its corresponding token type
    fn check_keyword(&self, pos: usize, len: usize, rest: &str, token_type: TokenType) -> TokenType {
        let partial_text: String = self.source.chars()
            .skip(self.start + pos)
            .take(len)
            .collect();

        if self.current - self.start == pos + len && rest == partial_text {
            return token_type
        }

        TokenType::Identifier
    }

    /// Finds the type of the given identifier string
    fn identifier_type(&mut self) -> TokenType {
        // Get current character
        let char = self.source.chars().nth(self.start).unwrap();

        // Match it with a list of keyword beginnings
        match char {
            'c' => self.check_keyword(1, 4, "lass", TokenType::Class),
            'e' => self.check_keyword(1, 3, "lse", TokenType::Else),
            'f' => {
                if (self.current - self.start > 1) {
                    let m_char = self.source.chars().nth(self.start + 1).unwrap();
                    match m_char {
                        'a' => self.check_keyword(2, 3, "lse", TokenType::False),
                        'o' => self.check_keyword(2, 1, "r", TokenType::For),
                        'u' => self.check_keyword(2, 2, "nc", TokenType::Func),
                        _ => TokenType::Identifier
                    }
                } else {
                    TokenType::Identifier
                }
            },
            'i' => self.check_keyword(1, 1, "f", TokenType::If),
            'n' => self.check_keyword(1, 3, "ull", TokenType::Null),
            'p' => self.check_keyword(1, 4, "rint", TokenType::Print),
            'r' => self.check_keyword(1, 5, "eturn", TokenType::Return),
            's' => self.check_keyword(1, 4, "uper", TokenType::Super),
            't' => {
                if self.current - self.start > 1 {
                    let m_char = self.source.chars().nth(self.start + 1).unwrap();
                    match m_char {
                        'h' => self.check_keyword(2, 2, "is", TokenType::This),
                        'r' => self.check_keyword(2, 2, "ue", TokenType::True),
                        _ => TokenType::Identifier
                    }
                } else {
                    TokenType::Identifier
                }
            },
            'v' => self.check_keyword(1, 2, "ar", TokenType::Var),
            'w' => self.check_keyword(1, 4, "hile", TokenType::While),
            _ => TokenType::Identifier
        }
    }

    /// Creates a new identifier token
    fn identifier(&mut self) -> Token {
        while self.peek().is_alphanumeric() {
            _ = self.advance();
        }

        let token_type = self.identifier_type();
        self.make_token(token_type)
    }

    /// Creates a new number token
    fn number(&mut self) -> Token {
        while self.peek().is_numeric() {
            _ = self.advance()
        }

        if self.peek() == '.' && self.peek_next().is_numeric() {
            _ = self.advance();
            while self.peek().is_numeric() {
                _ = self.advance();
            }
        }

        self.make_token(TokenType::Number)
    }

    /// Creates a new string token
    fn string(&mut self, open_char: char) -> Token {
        while self.peek() != open_char && !self.is_at_end() {
            if self.peek() == '\n' {
                self.line += 1;
            }
            
            _ = self.advance();
        }

        if self.is_at_end() {
            return self.error_token("Unterminated string literal!")
        }

        // Closing string delimiter
        _ = self.advance();
        self.make_token(TokenType::String)
    }
}

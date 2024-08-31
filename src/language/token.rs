
/// Describes the type of a Token.
pub enum TokenType {
    // Single-character tokens.
    OpenParen, CloseParen,
    OpenCurly, CloseCurly,
    Comma, Dot, Minus, Plus,
    Semicolon, Slash, Star,
    
    // One or two character tokens.
    Excl, ExclEqual,
    Equal, EqualEqual,
    Greater, GreaterEqual,
    Less, LessEqual,
    
    // Literals.
    Identifier, String, Number,
    
    // Keywords.
    And, Class, Else, False,
    Func, For, If, Null, Or,
    Print, Return, Super, This,
    True, Var, While,
    
    Error,
    Eof,
    
    // Bitwise operations
    BwAnd, BwOr, BwXor, BwNot,

    // Array index notation
    OpenSquare, CloseSquare,

    // Bitwise shifts
    GreaterGreater, LessLess
}

/// Describes a token in the elysabettian programming language
pub struct Token {
    pub token_type: TokenType,
    pub text: String,
    pub line: i32
}

impl Token {
    /// Create a new Token.
    /// 
    /// Arguments:
    /// * `token_type`: type of this token
    /// * `text`: reference to string of text (content) 
    /// * `line`: line number associated with this token
    pub fn new(token_type: TokenType, text: &String, line: i32) -> Token {
        Token {
            token_type,
            text: text.clone(),
            line
        }
    }
}
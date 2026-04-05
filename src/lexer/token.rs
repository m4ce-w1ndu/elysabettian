use logos::Logos;

/// All token types in the Elysabettian language.
///
/// Produced by the lexer and consumed by the parser. Each variant corresponds
/// directly to a syntactic element: a keyword, an operator, a literal, or a
/// structural marker (`Eof`, `Error`).
///
/// Derived via [`logos::Logos`], which generates a DFA-based lexer at compile
/// time from the annotations on each variant. `#[token]` matches a fixed
/// string; `#[regex]` matches a pattern. When both could match, `#[token]`
/// takes priority — this is how keywords are distinguished from identifiers
/// without any manual trie logic.
#[derive(Logos, Debug, PartialEq, Clone)]
#[logos(skip r"[ \t\r\n]+")]
#[logos(skip r"//[^\n]*")]
pub enum Token {
    // Keywords
    #[token("class")]
    Class,
    #[token("else")]
    Else,
    #[token("false")]
    False,
    #[token("for")]
    For,
    #[token("func")]
    Func,
    #[token("if")]
    If,
    #[token("null")]
    Null,
    #[token("print")]
    Print,
    #[token("return")]
    Return,
    #[token("super")]
    Super,
    #[token("this")]
    This,
    #[token("true")]
    True,
    #[token("var")]
    Var,
    #[token("while")]
    While,

    // Literals
    /// A floating-point number literal. The value is parsed eagerly at lex time.
    #[regex(r"[0-9]+(\.[0-9]+)?", |lex| lex.slice().parse::<f64>().ok())]
    Number(f64),

    /// A string literal. Supports both double-quoted and single-quoted forms.
    /// The surrounding quotes are stripped; the inner content is stored.
    #[regex(r#""[^"]*""#, |lex| lex.slice().trim_matches('"').to_string())]
    #[regex(r#"'[^']*'"#, |lex| lex.slice().trim_matches('\'').to_string())]
    String(String),

    /// An identifier. Matches any name not claimed by a keyword token.
    /// This includes built-in function names like `import`, `clock`, and `string`.
    #[regex(r"[a-zA-Z_][a-zA-Z0-9_]*", |lex| lex.slice().to_string())]
    Identifier(String),

    // Single-character tokens
    #[token("(")]
    OpenParen,
    #[token(")")]
    CloseParen,
    #[token("{")]
    OpenCurly,
    #[token("}")]
    CloseCurly,
    #[token("[")]
    OpenSquare,
    #[token("]")]
    CloseSquare,
    #[token(",")]
    Comma,
    #[token(".")]
    Dot,
    #[token("-")]
    Minus,
    #[token("+")]
    Plus,
    #[token("/")]
    Slash,
    #[token("*")]
    Star,
    #[token(";")]
    Semicolon,
    #[token("^")]
    BwXor,

    // One- or two-character tokens
    /// `!` — logical not.
    #[token("!")]
    Excl,
    /// `!=` — inequality.
    #[token("!=")]
    ExclEqual,
    /// `=` — assignment.
    #[token("=")]
    Equal,
    /// `==` — equality comparison.
    #[token("==")]
    EqualEqual,
    /// `>` — greater than.
    #[token(">")]
    Greater,
    /// `>=` — greater than or equal.
    #[token(">=")]
    GreaterEqual,
    /// `>>` — bitwise right shift.
    #[token(">>")]
    GreaterGreater,
    /// `<` — less than.
    #[token("<")]
    Less,
    /// `<=` — less than or equal.
    #[token("<=")]
    LessEqual,
    /// `<<` — bitwise left shift.
    #[token("<<")]
    LessLess,
    /// `&&` — logical and (short-circuit).
    #[token("&&")]
    And,
    /// `||` — logical or (short-circuit).
    #[token("||")]
    Or,
    /// `&` — bitwise and.
    #[token("&")]
    BwAnd,
    /// `|` — bitwise or.
    #[token("|")]
    BwOr,

    // Structural markers
    /// Marks the end of the token stream. Emitted once when the source is exhausted.
    ///
    /// `logos` does not emit this automatically — the lexer must insert it after
    /// the iterator is exhausted. Lexing errors surface as `Err(())` in the
    /// iterator and are handled by the parser.
    Eof,
}

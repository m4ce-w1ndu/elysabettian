pub mod token;

use crate::lexer::token::Token;
use logos::Logos;

/// Wraps the `logos`-generated lexer and provides a peekable token stream
/// for the parser. Tracks the current source span for error reporting.
pub struct Lexer<'src> {
    inner: logos::Lexer<'src, Token>,
    /// The next token, already pulled from the iterator (one token lookahead).
    peeked: Option<Result<Token, ()>>,
    /// Byte span of the most recently consumed token.
    pub span: logos::Span,
}

impl<'src> Lexer<'src> {
    pub fn new(source: &'src str) -> Self {
        let mut inner = Token::lexer(source);
        let peeked = inner.next();
        Self {
            inner,
            peeked,
            span: 0..0,
        }
    }

    /// Returns a reference to the next token without consuming it.
    /// Returns `None` when the token stream is exhausted.
    /// Lexing errors (`Err(())`) are silently skipped — the parser sees `None`.
    pub fn peek(&self) -> Option<&Token> {
        self.peeked.as_ref()?.as_ref().ok()
    }

    /// Consumes and returns the next token.
    /// Returns `Token::Eof` when the stream is exhausted.
    pub fn next(&mut self) -> Token {
        self.span = self.inner.span();
        let current = self.peeked.take();
        self.peeked = self.inner.next();
        current.and_then(|r| r.ok()).unwrap_or(Token::Eof)
    }

    /// Consumes the next token if it matches `expected`. Returns whether it matched.
    pub fn eat(&mut self, expected: &Token) -> bool {
        if self.peek() == Some(expected) {
            self.next();
            true
        } else {
            false
        }
    }
}

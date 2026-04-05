use crate::lexer::Lexer;
use crate::lexer::token::Token;
use crate::parser::ast::{Expr, Stmt, StmtKind};

pub mod ast;
mod expressions;
mod statements;

/// This is the language's Parser. It holds the lexer state
/// and the current position in the tokens stream.
/// It builds the AST and readies everything for code generation.
pub struct Parser<'src> {
    lexer: Lexer<'src>,
    previous: Token,
    current: Token,
    had_error: bool,
    panic_mode: bool,
}

impl<'src> Parser<'src> {
    pub fn new(source: &'src str) -> Self {
        let mut parser = Self {
            lexer: Lexer::new(source),
            previous: Token::Eof,
            current: Token::Eof,
            had_error: false,
            panic_mode: false,
        };

        parser.advance();
        parser
    }

    /// Advances the parser to the next token in the stream.
    fn advance(&mut self) {
        self.previous = self.current.clone();
        self.current = self.lexer.next();
    }

    /// Consumes the token, performing basic syntax checking.
    fn consume(&mut self, expected: &Token, message: &str) {
        if &self.current == expected {
            self.advance();
            return;
        }

        self.error(message);
    }

    /// Parses a print statement.
    fn parse_print(&mut self) -> Stmt {
        let value = self.expression();
        self.consume(&Token::Semicolon, "Expected ';' after value.");

        Stmt {
            kind: StmtKind::Print(Box::new(value)),
            span: self.lexer.span.clone(),
        }
    }

    fn error(&mut self, message: &str) {
        todo!();
    }
}

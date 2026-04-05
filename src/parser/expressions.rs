use crate::parser::ast::{ExprKind, Literal};

use super::*;

/// Defines how the parser should treat expressions precedence.
#[derive(Debug, Clone, PartialEq, PartialOrd)]
enum Precedence {
    None,
    Assignment,
    Or,
    And,
    Equality,
    Comparison,
    Term,
    Factor,
    Unary,
    Call,
    Primary,
}

impl<'src> Parser<'src> {
    pub(super) fn expression(&mut self) -> Expr {
        todo!();
    }

    /// Parses the precedence of the current expression, building the expression tree.
    pub(super) fn parse_precedence(&mut self, min_prec: Precedence) -> Expr {
        self.advance();
        let mut left = self.prefix_rule(&self.previous.clone());

        while get_precedence(&self.current) >= min_prec {
            self.advance();
            left = self.infix_rule(&self.previous.clone(), left);
        }

        left
    }

    pub(super) fn prefix_rule(&mut self, token: &Token) -> Expr {
        todo!();
    }

    pub(super) fn infix_rule(&mut self, token: &Token, expr: Expr) -> Expr {
        todo!();
    }

    /// Parses a number literal from the token stream.
    pub(super) fn parse_number(&mut self) -> Expr {
        let Token::Number(value) = self.previous.clone() else {
            unreachable!()
        };

        Expr {
            kind: ExprKind::Literal(Literal::Number(value)),
            span: self.lexer.span.clone(),
        }
    }

    /// Parses a string literal from the token stream.
    pub(super) fn parse_string(&mut self) -> Expr {
        let Token::String(string) = self.previous.clone() else {
            unreachable!()
        };

        Expr {
            kind: ExprKind::Literal(Literal::String(string)),
            span: self.lexer.span.clone(),
        }
    }

    /// Parses a unary expression from the token stream.
    pub(super) fn parse_unary(&mut self) -> Expr {
        todo!()
    }
}

/// Returns the precedence of the expression associated with a specific token.
pub(super) fn get_precedence(token: &Token) -> Precedence {
    match token {
        Token::Plus | Token::Minus => Precedence::Term,
        Token::Star | Token::Slash => Precedence::Factor,
        Token::EqEq | Token::ExclEq => Precedence::Equality,
        Token::Lt | Token::Gt | Token::LtEq | Token::GtEq => Precedence::Comparison,
        Token::And => Precedence::And,
        Token::Or => Precedence::Or,
        Token::OpenParen | Token::Dot | Token::OpenSquare => Precedence::Call,
        Token::LtLt | Token::GtGt => Precedence::Term,
        Token::BwAnd | Token::BwOr | Token::BwXor => Precedence::Term,
        _ => Precedence::None,
    }
}

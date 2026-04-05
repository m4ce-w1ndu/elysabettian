use std::ops::Range;

/// A byte range into the original source string, used for error reporting.
/// Comes directly from `lexer.span()` after consuming a token.
pub type Span = Range<usize>;

/// A literal value as it appears in source code.
#[derive(Debug, Clone)]
pub enum Literal {
    Number(f64),
    String(String),
    Bool(bool),
    Null,
}

/// The operator in a binary expression
#[derive(Debug, Clone)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Eq,
    NotEq,
    Lt,
    Gt,
    LtEq,
    GtEq,
    And,
    Or,
    BwAnd,
    BwOr,
    BwXor,
    Shl,
    Shr,
}

/// A single expression node, with the source span it came from.
#[derive(Debug, Clone)]
pub struct Expr {
    pub kind: ExprKind,
    pub span: Span,
}

/// The kind of expression that the parser is evaluating.
#[derive(Debug, Clone)]
pub enum ExprKind {
    /// Any literal value: `42`, `"hello"`, `true`, `null`
    Literal(Literal),
    Binary {
        left: Box<Expr>,
        op: BinaryOp,
        right: Box<Expr>,
    },
}

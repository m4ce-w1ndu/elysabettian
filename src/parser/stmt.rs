use crate::{error::Span, parser::expr::Expr};

#[derive(Debug, Clone)]
pub struct Stmt {
    pub kind: StmtKind,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub enum StmtKind {
    Expression(Box<Expr>),
}

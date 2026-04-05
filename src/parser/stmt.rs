use crate::{
    error::Span,
    parser::{decl::Decl, expr::Expr},
};

#[derive(Debug, Clone)]
pub struct Stmt {
    pub kind: StmtKind,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub enum StmtKind {
    /// Block statement - a block of code.
    Block(Vec<Decl>),

    /// Expression statement - an expression.
    Expression(Box<Expr>),

    /// Variable declaration - var x = 0; var x;
    VarDecl {
        name: String,
        initialiser: Option<Box<Expr>>,
    },

    /// For statement - for (var i = 0; i < 10; i = i + 1).
    For {
        initialiser: Option<Box<Stmt>>, // var i = 0;
        condition: Option<Box<Expr>>,   // i < 10;
        increment: Option<Box<Expr>>,   // i = i + 1
        body: Box<Stmt>,
    },

    /// If statement - if (pippo) {} else {}
    If {
        condition: Box<Expr>,
        then_branch: Box<Stmt>,
        else_branch: Option<Box<Stmt>>,
    },

    /// While statement - while (cond) {}
    While {
        condition: Box<Expr>,
        body: Box<Stmt>,
    },

    /// Print statement - print "Hello";
    Print(Box<Expr>),

    /// Return statement - return "Hello";
    Return(Option<Box<Expr>>),
}

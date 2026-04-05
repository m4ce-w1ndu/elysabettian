use crate::{error::Span, parser::ast::stmt::Stmt};

/// A declaration - it can be a function declaration,
/// a class declaration or a simple declaration statement
/// for a single variable.
#[derive(Debug, Clone)]
pub struct Decl {
    pub kind: DeclKind,
    pub span: Span,
}

/// Method declaration - similar to a function's declaration, but
/// a different type, so that it will be easier to bind it with
/// its corresponding bytecode emission.
#[derive(Debug, Clone)]
pub struct MethodDecl {
    pub name: String,
    pub params: Vec<String>,
    pub body: Vec<Decl>,
}

/// Kind of declaration.
#[derive(Debug, Clone)]
pub enum DeclKind {
    /// Function declaration - func square(x).
    Function {
        name: String,
        params: Vec<String>,
        body: Vec<Decl>,
    },

    /// Class declaration - class Point {}
    Class {
        name: String,
        superclass: Option<String>,
        methods: Vec<MethodDecl>,
    },

    /// Simple statement - such as a variable.
    Stmt(Stmt),
}

use crate::error::Span;

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

/// The operator in a unary expression.
#[derive(Debug, Clone)]
pub enum UnaryOp {
    Not,
    Neg,
    BwNot,
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

    ///Variable identifiers.
    Variable(String),

    /// Array literal.
    Array(Vec<Expr>),

    /// Reading an array element with its index.
    ArrayIndex { array: Box<Expr>, index: Box<Expr> },

    /// Storing a value into an array element with its index.
    /// arr[i] = value.
    ArrayStore {
        array: Box<Expr>,
        index: Box<Expr>,
        value: Box<Expr>,
    },

    /// Any binary expression with a logical or mathematical operator.
    Binary {
        left: Box<Expr>,
        op: BinaryOp,
        right: Box<Expr>,
    },

    /// Unary expression, with its operand and operator.
    Unary { op: UnaryOp, operand: Box<Expr> },

    /// obj.field - just reading a property's value.
    Get { object: Box<Expr>, field: String },

    /// obj.field = value - just writing a property's value.
    Set {
        object: Box<Expr>,
        field: String,
        value: Box<Expr>,
    },

    /// obj.method(a, b) - calling a method directly
    /// kept separate from Get + Call because the VM has a dedicated
    /// Invoke opcode that is faster than get-then-call.
    Invoke {
        object: Box<Expr>,
        method: String,
        args: Vec<Expr>,
    },

    /// Assignment expression.
    Assign { name: String, value: Box<Expr> },

    /// Function call expression.
    Call { callee: Box<Expr>, args: Vec<Expr> },

    /// Reference to current object.
    This,

    /// super.method_name() - I just saw a method.
    Super(String),
}

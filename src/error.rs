use std::ops::Range;

/// A byte range into the original source string, used for error reporting.
/// Comes directly from `lexer.span()` after consuming a token.
pub type Span = Range<usize>;

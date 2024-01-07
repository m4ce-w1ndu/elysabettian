namespace Infrastructure.Language
{
    /// <summary>
    /// Describes the type of a Token.
    /// </summary>
    public enum TokenType
    {
        // Single-character tokens.
        OpenParen, CloseParen,
        OpenCurly, CloseCurly,
        Comma, Dot, Minus, Plus,
        Semicolon, Slash, Star,

        // One or two character tokens.
        Excl, ExclEqual,
        Equal, EqualEqual,
        Greater, GreaterEqual,
        Less, LessEqual,

        // Literals.
        Identifier, String, Number,

        // Keywords.
        And, Class, Else, False,
        Func, For, If, Null, Or,
        Print, Return, Super, This,
        True, Var, While,

        Error,
        Eof,

        // Bitwise operations
        Ampersand, Pipe, Caret, Tilde,

        // Array index notation
        OpenSquare, CloseSquare,
    }

    /// <summary>
    /// Describes a Token in the Elysabettian language.
    /// </summary>
    /// <param name="type">Type of this token.</param>
    /// <param name="text">Text content of this token.</param>
    /// <param name="line">Line number of this token.</param>
    public struct Token(TokenType type, string text, int line)
    {
        public TokenType Type { get; set; } = type;

        public string Text { get; set; } = text;

        public int Line { get; set; } = line;
    }
}

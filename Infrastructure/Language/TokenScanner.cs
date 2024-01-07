namespace Infrastructure.Language
{
    /// <summary>
    /// Tokenizes the source code.
    /// </summary>
    /// <param name="source">Elysabettian source code.</param>
    public class TokenScanner(string source)
    {
        /// <summary>
        /// Source code.
        /// </summary>
        private string source = source;

        /// <summary>
        /// Start of the tokenizer.
        /// </summary>
        private int start = 0;

        /// <summary>
        /// Current position of the tokenizer.
        /// </summary>
        private int current = 0;

        /// <summary>
        /// Current line of the tokenizer.
        /// </summary>
        private int line = 1;

        public Token Scan()
        {
            SkipWhitespace();
            start = current;

            if (IsAtEnd()) return MakeToken(TokenType.Eof);

            char c = Advance();
            if (char.IsDigit(c)) return Number();
            if (char.IsLetter(c)) return Identifier();

            return c switch
            {
                '(' => MakeToken(TokenType.OpenParen),
                ')' => MakeToken(TokenType.CloseParen),
                '[' => MakeToken(TokenType.OpenSquare),
                ']' => MakeToken(TokenType.CloseSquare),
                '{' => MakeToken(TokenType.OpenCurly),
                '}' => MakeToken(TokenType.CloseCurly),
                ';' => MakeToken(TokenType.Semicolon),
                ',' => MakeToken(TokenType.Comma),
                '.' => MakeToken(TokenType.Dot),
                '-' => MakeToken(TokenType.Minus),
                '+' => MakeToken(TokenType.Plus),
                '/' => MakeToken(TokenType.Slash),
                '*' => MakeToken(TokenType.Star),
                '^' => MakeToken(TokenType.Caret),
                '&' => MakeToken(Match('&') ? TokenType.And : TokenType.Ampersand),
                '|' => MakeToken(Match('|') ? TokenType.Or : TokenType.Pipe),
                '!' => MakeToken(Match('=') ? TokenType.ExclEqual : TokenType.Excl),
                '=' => MakeToken(Match('=') ? TokenType.EqualEqual : TokenType.Equal),
                '<' => MakeToken(Match('=') ? TokenType.LessEqual : TokenType.Less),
                '>' => MakeToken(Match('=') ? TokenType.GreaterEqual : TokenType.Greater),
                '"' => String(),
                '\'' => String('\''),
                _ => ErrorToken("Unexpected character.")
            };
        }

        /// <summary>
        /// Check if the tokenizer reached end of source code.
        /// </summary>
        private bool IsAtEnd() => current == source.Length;

        /// <summary>
        /// Advance the tokenizer.
        /// </summary>
        private char Advance()
        {
            current++;
            return source[current - 1];
        }

        /// <summary>
        /// Read current character.
        /// </summary>
        private char Peek()
        {
            if (IsAtEnd()) return '\0';
            return source[current];
        }

        /// <summary>
        /// Read next character.
        /// </summary>
        private char PeekNext()
        {
            if ((current + 1) >= source.Length) return '\0';
            return source[current + 1];
        }

        /// <summary>
        /// Match current character.
        /// </summary>
        private bool Match(char expected)
        {
            if (IsAtEnd()) return false;
            if (source[current] != expected) return false;

            current++;
            return true;
        }

        /// <summary>
        /// Make a new token.
        /// </summary>
        private Token MakeToken(TokenType type)
        {
            string text = source[start..current];
            return new Token(type, text, line);
        }

        /// <summary>
        /// Make an error token.
        /// </summary>
        private Token ErrorToken(string message)
        {
            return new Token(TokenType.Error, message, line);
        }

        /// <summary>
        /// Skip whitespaces in source code.
        /// </summary>
        private void SkipWhitespace()
        {
            while (true)
            {
                char c = Peek();
                switch (c)
                {
                    case ' ':
                    case '\r':
                    case '\t':
                        Advance();
                        break;
                    case '\n':
                        line++;
                        Advance();
                        break;
                    case '/':
                        if (PeekNext() != '/') return;
                        while (Peek() != '\n' && !IsAtEnd()) Advance();
                        break;
                    default:
                        return;
                }
            }
        }

        /// <summary>
        /// Check for a keyword.
        /// </summary>
        private TokenType CheckKeyword(int pos, int len, string rest, TokenType type)
        {
            if (current - start == pos + len && source.Substring(start + pos, len) == rest)
                return type;
            return TokenType.Identifier;
        }

        /// <summary>
        /// Check the type of the indentifier.
        /// </summary>
        private TokenType IdentifierType()
        {
            return source[start] switch
            {
                'c' => CheckKeyword(1, 4, "lass", TokenType.Class),
                'e' => CheckKeyword(1, 3, "lse", TokenType.Else),
                'f' => current - start > 1 ? source[start + 1] switch
                {
                    'a' => CheckKeyword(2, 3, "lse", TokenType.False),
                    'o' => CheckKeyword(2, 1, "r", TokenType.For),
                    'u' => CheckKeyword(2, 2, "nc", TokenType.Func),
                    _ => TokenType.Identifier
                } : TokenType.Identifier,
                'i' => CheckKeyword(1, 1, "f", TokenType.If),
                'n' => CheckKeyword(1, 3, "ull", TokenType.Null),
                'p' => CheckKeyword(1, 4, "rint", TokenType.Print),
                'r' => CheckKeyword(1, 5, "eturn", TokenType.Return),
                's' => CheckKeyword(1, 4, "uper", TokenType.Super),
                't' => current - start > 1 ? source[start + 1] switch
                {
                    'h' => CheckKeyword(2, 2, "is", TokenType.This),
                    'r' => CheckKeyword(2, 2, "ue", TokenType.True),
                    _ => TokenType.Identifier
                } : TokenType.Identifier,
                'v' => CheckKeyword(1, 2, "ar", TokenType.Var),
                'w' => CheckKeyword(1, 4, "hile", TokenType.While),
                _ => TokenType.Identifier
            };
        }

        /// <summary>
        /// Tokenize an identifier.
        /// </summary>
        private Token Identifier()
        {
            while (char.IsLetterOrDigit(Peek())) Advance();
            return MakeToken(IdentifierType());
        }

        /// <summary>
        /// Tokenize a number.
        /// </summary>
        private Token Number()
        {
            while (char.IsDigit(Peek())) Advance();

            if (Peek() == '.' && char.IsDigit(PeekNext()))
            {
                Advance();
                while (char.IsDigit(Peek())) Advance();
            }

            return MakeToken(TokenType.Number);
        }

        /// <summary>
        /// Tokenize a string.
        /// </summary>
        private Token String(char delimiter = '\'')
        {
            while (Peek() != delimiter && !IsAtEnd())
            {
                if (Peek() != '\n') line++;
                Advance();
            }

            if (IsAtEnd()) return ErrorToken("Unterminated string.");

            Advance();
            return MakeToken(TokenType.String);
        }
    }
}

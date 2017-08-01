const char OPERATORS[256 - 32] = {
  0, 1, 2, 2, 2, 1, 1, 2,  //  !"#$%&'
  2, 2, 1, 1, 2, 1, 2, 1,  // ()*+,-./
  0, 0, 0, 0, 0, 0, 0, 0,  // 01234567
  0, 0, 3, 2, 1, 3, 1, 1,  // 89:;<=>?
  3, 0, 0, 0, 0, 0, 0, 0,  // @ABCDEFG
  0, 0, 0, 0, 0, 0, 0, 0,  // HIJKLMNO
  0, 0, 0, 0, 0, 0, 0, 0,  // PQRSTUVW
  0, 0, 0, 2, 2, 2, 1, 0,  // XYZ[\]^_
  1, 0, 0, 0, 0, 0, 0, 0,  // `abcdefg
  0, 0, 0, 0, 0, 0, 0, 0,  // hijklmno
  0, 0, 0, 0, 0, 0, 0, 0,  // pqrstuvw
  0, 0, 0, 2, 1, 2, 1, 0,  // xyz{|}~
};

// @Precondition: file data is never freed.
// @Precondition: input data is never freed.
TokenizedFile* tokenize_string(String* file, String* input) {
  // @Lazy: We're allocating as much memory as we could ever possibly need, in
  // the hopes that we can avoid ever having to reallocate mid-routine.  This is
  // likely *way* more memory than we actually need, but it's at least cheap and
  // fairly reliable.
  size_t input_length = input->length;
  Token* tokens = calloc(input_length + 1, sizeof(Token));
  String* lines = calloc(input_length, sizeof(String));
  size_t token_idx = 0;

  size_t token_start = 0; // Position in the file where the current token began.
  size_t file_pos = 0;    // Position in the file we're currently parsing.

  size_t line_no = 0;     // Line number we're parsing.
  size_t line_pos = 0;    // Position within the line we're parsing.
  size_t line_start = 0;  // File offset for beginning of the current line.

  #define THIS  (input->data[file_pos])
  #define LAST  (input->data[file_pos - 1])
  #define NEXT  (file_pos + 1 < input->length ? input->data[file_pos + 1] : '\0')
  #define LENGTH  (file_pos - token_start)
  #define SOURCE  (*substring(input, token_start, LENGTH))

  #define IS_WHITESPACE(T)    (T == ' ' || T == '\t')
  #define IS_NEWLINE(T)       (T == '\n')
  #define IS_BINARY_DIGIT(T)  (T == '_' || T == '0' || T == '1')
  #define IS_DECIMAL_DIGIT(T) (T == '_' || (T >= '0' && T <= '9'))
  #define IS_HEX_DIGIT(T)     (IS_DECIMAL_DIGIT(T) || (T >= 'a' && T <= 'z') || (T >= 'A' && T <= 'Z'))
  #define IS_OPERATOR(T)      (OPERATORS[T - 32] == 1)
  #define IS_RESERVED_OP(T)   (OPERATORS[T - 32] == 2)
  #define IS_NONINITIAL_OP(T) (OPERATORS[T - 32])
  #define IS_IDENTIFIER(T)    (!(IS_WHITESPACE(T) || IS_NEWLINE(T) || IS_OPERATOR(T) || IS_RESERVED_OP(T)))

  #define ADVANCE(EXPECTED)   do { assert(EXPECTED == THIS); file_pos += 1; line_pos += 1; } while (0)

  #define SLURP(COND)             while (file_pos < input_length && COND) { ADVANCE(THIS); }
  #define SLURP_WHITESPACE()      SLURP(IS_WHITESPACE(THIS))
  #define SLURP_NEWLINES()        SLURP(IS_NEWLINE(THIS))
  #define SLURP_TO_EOL()          SLURP(!IS_NEWLINE(THIS))
  #define SLURP_COMMENT()         SLURP(IS_NEWLINE(THIS))
  #define SLURP_BINARY_NUMBER()   SLURP(IS_BINARY_DIGIT(THIS))
  #define SLURP_DECIMAL_NUMBER()  SLURP(IS_DECIMAL_DIGIT(THIS))
  #define SLURP_HEX_NUMBER()      SLURP(IS_HEX_DIGIT(THIS))
  #define SLURP_OPERATOR()        SLURP(IS_NONINITIAL_OP(THIS))
  #define SLURP_IDENT()           SLURP(IS_IDENTIFIER(THIS))

  #define START()    (token_start = file_pos)

  while (file_pos < input_length) {
    token_start = file_pos;

    switch (THIS) {
      case '@':
        ADVANCE('@');
        SLURP_IDENT();
        tokens[token_idx++] = (Token) { TOKEN_DIRECTIVE, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case '#':
        ADVANCE('#');
        SLURP_IDENT();
        tokens[token_idx++] = (Token) { TOKEN_TAG, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case ' ':
      case '\t':
        SLURP_WHITESPACE();
        // @TODO Figure out how to differentiate `ident (` from `ident(`.  Do
        //       we really even care?
        break;
      case '\n':
        {
          size_t _line_no = line_no;
          while (file_pos < input_length && IS_NEWLINE(THIS)) {
            lines[_line_no++] = *substring(input, line_start, file_pos - line_start);
            line_start = file_pos + 1;
            ADVANCE(THIS);
            SLURP_WHITESPACE();
          }

          tokens[token_idx++] = (Token) { TOKEN_NEWLINE, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };

          line_no = _line_no;
          line_pos = 0;
          break;
        }
      case '"':
        do {
          ADVANCE('"');
          SLURP(THIS != '"' && !IS_NEWLINE(THIS));
        } while (LAST == '\\' && !IS_NEWLINE(THIS));

        if (file_pos < input_length && !IS_NEWLINE(THIS)) {
          ADVANCE('"');
        }

        tokens[token_idx++] = (Token) { TOKEN_LITERAL, *file, line_no, line_pos - LENGTH, SOURCE, LITERAL_STRING, input->data[token_start] == input->data[file_pos - 1] };
        break;
      case '0'...'9':
        {
          TokenLiteralType literal_type = NONLITERAL;

          if (THIS == '0') {
            ADVANCE('0');
            if (THIS == 'x') {
              literal_type = NUMBER_HEX;
              ADVANCE('x');
              SLURP_HEX_NUMBER();
            } else if (THIS == 'b') {
              literal_type = NUMBER_BINARY;
              ADVANCE('b');
              SLURP_BINARY_NUMBER();
            }
          }

          if (!literal_type) {
            literal_type = NUMBER_DECIMAL;
            SLURP_DECIMAL_NUMBER();

            if (THIS == '.') {
              literal_type = NUMBER_FRACTION;
              ADVANCE('.');
              SLURP_DECIMAL_NUMBER();
            }
          }

          tokens[token_idx++] = (Token) { TOKEN_LITERAL, *file, line_no, line_pos - LENGTH, SOURCE, literal_type, 1 };
        }
        break;
      case ',':
      case '(':
      case ')':
      case '{':
      case '}':
        ADVANCE(THIS);
        tokens[token_idx++] = (Token) { TOKEN_SYNTAX_OPERATOR, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case ':':
        SLURP_OPERATOR();
        tokens[token_idx++] = (Token) { TOKEN_SYNTAX_OPERATOR, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case '/':
        if (NEXT == '/') {
          SLURP_TO_EOL();
          // @TODO This can potentially lead to back-to-back newlines, despite
          //       our best efforts.
          // tokens[token_idx++] = (Token) { TOKEN_COMMENT, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
          break;
        } else {
          // Continue to process as an operator.
        }
      case '!':
      case '`':
      case '$'...'\'':
      case '*'...'+':
      case '-'...'.':
      case ';'...'?':
      case '['...'^':
      case '|':
      case '~':
        SLURP_OPERATOR();
        tokens[token_idx++] = (Token) { TOKEN_OPERATOR, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      default:
        SLURP_IDENT();
        tokens[token_idx++] = (Token) { TOKEN_IDENTIFIER, *file, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
    }
  }

  TokenizedFile* result = malloc(sizeof(TokenizedFile));
  result->file = *file;

  if (token_idx) {
    if (tokens[token_idx - 1].type != TOKEN_NEWLINE) {
      tokens[token_idx++] = (Token) { TOKEN_IDENTIFIER, *file, line_no, line_pos - LENGTH, *new_string("\n"), NONLITERAL, 1 };
      token_idx += 1;
    }

    result->count = token_idx;
    result->tokens = realloc(tokens, result->count * sizeof(Token));
    result->lines = realloc(lines, result->count * sizeof(String));
  } else {
    result->count = 0;
    result->tokens = NULL;
    result->lines = NULL;
    free(tokens);
    free(lines);
  }

  #undef THIS
  #undef LAST
  #undef NEXT
  #undef LENGTH
  #undef SOURCE
  #undef IS_WHITESPACE
  #undef IS_NEWLINE
  #undef IS_BINARY_DIGIT
  #undef IS_DECIMAL_DIGIT
  #undef IS_HEX_DIGIT
  #undef IS_OPERATOR
  #undef IS_IDENTIFIER
  #undef ADVANCE
  #undef SLURP
  #undef SLURP_WHITESPACE
  #undef SLURP_NEWLINES
  #undef SLURP_TO_EOL
  #undef SLURP_COMMENT
  #undef SLURP_BINARY_NUMBER
  #undef SLURP_DECIMAL_NUMBER
  #undef SLURP_HEX_NUMBER
  #undef SLURP_OPERATOR
  #undef SLURP_IDENT
  #undef START

  return result;
}

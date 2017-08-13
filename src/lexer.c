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
void tokenize_string(FileInfo* file, TokenizedFile* result) {
  String* input = file->source;

  size_t input_length = input->length;
  Pool* tokens = new_pool(sizeof(Token), 128, 32);
  Pool* lines = new_pool(sizeof(String), 128, 32);

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
  #define IS_IDENTIFIER(T)    (!(IS_WHITESPACE(T) || IS_NEWLINE(T) || IS_NONINITIAL_OP(T)))

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
        *((Token*) pool_get(tokens)) = (Token) { TOKEN_DIRECTIVE, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case '#':
        ADVANCE('#');
        SLURP_IDENT();
        *((Token*) pool_get(tokens)) = (Token) { TOKEN_TAG, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case ' ':
      case '\t':
        SLURP_WHITESPACE();
        // @TODO Figure out how to differentiate `ident (` from `ident(`.  Do
        //       we really even care?
        break;
      case '\n':
        {
          // while (file_pos < input_length && IS_NEWLINE(THIS)) {
          //   *((String*) pool_get(lines)) = (String) { file_pos - line_start, input->data + line_start };
          //   line_no += 1;
          //   line_start = file_pos + 1;
          //   ADVANCE(THIS);
          //   SLURP_WHITESPACE();
          // }

          ADVANCE(THIS);
          *((String*) pool_get(lines)) = (String) { file_pos - line_start - 1, input->data + line_start };
          *((Token*) pool_get(tokens)) = (Token) { TOKEN_SYNTAX_OPERATOR, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
          line_no += 1;
          line_start = file_pos;
          line_pos = 0;
          SLURP_WHITESPACE();
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

        *((Token*) pool_get(tokens)) = (Token) { TOKEN_LITERAL, line_no, line_pos - LENGTH, SOURCE, IS_STRING_LITERAL, input->data[token_start] == input->data[file_pos - 1] };
        break;
      case '0'...'9':
        {
          TokenLiteralType literal_type = NONLITERAL;

          if (THIS == '0') {
            ADVANCE('0');
            if (THIS == 'x') {
              literal_type = IS_HEX_LITERAL;
              ADVANCE('x');
              SLURP_HEX_NUMBER();
            } else if (THIS == 'b') {
              literal_type = IS_BINARY_LITERAL;
              ADVANCE('b');
              SLURP_BINARY_NUMBER();
            }
          }

          if (!literal_type) {
            literal_type = IS_DECIMAL_LITERAL;
            SLURP_DECIMAL_NUMBER();

            if (THIS == '.') {
              literal_type = IS_FRACTIONAL_LITERAL;
              ADVANCE('.');
              SLURP_DECIMAL_NUMBER();
            }
          }

          // @TODO Parse numeric literals more loosely, reporting malformed literals.
          *((Token*) pool_get(tokens)) = (Token) { TOKEN_LITERAL, line_no, line_pos - LENGTH, SOURCE, literal_type, 1 };
        }
        break;
      case ',':
      case '(':
      case ')':
      case '{':
      case '}':
        ADVANCE(THIS);
        *((Token*) pool_get(tokens)) = (Token) { TOKEN_SYNTAX_OPERATOR, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case ':':
        SLURP_OPERATOR();
        *((Token*) pool_get(tokens)) = (Token) { TOKEN_SYNTAX_OPERATOR, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      case '/':
        if (NEXT == '/') {
          SLURP_TO_EOL();
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
        *((Token*) pool_get(tokens)) = (Token) { TOKEN_OPERATOR, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
        break;
      default:
        SLURP_IDENT();
        *((Token*) pool_get(tokens)) = (Token) { TOKEN_IDENTIFIER, line_no, line_pos - LENGTH, SOURCE, NONLITERAL, 1 };
    }
  }

  // @TODO Do we need to make sure the token stream ends with a newline?
  result->length = tokens->length;
  result->tokens = pool_to_array(tokens);

  file->length = lines->length;
  file->lines = pool_to_array(lines);

  free(tokens);
  free(lines);

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
}


bool perform_lex_job(Job* job) {
  TokenizedFile* result = malloc(sizeof(TokenizedFile));

  tokenize_string(job->file, result);
  pipeline_emit_parse_job(job->ws, job->file, result);
  return 1;
}

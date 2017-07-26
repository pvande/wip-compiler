typedef enum {
  TOKEN_TYPE_WHITESPACE,
  TOKEN_TYPE_NEWLINE,
  TOKEN_TYPE_COMMENT,
  TOKEN_TYPE_NUMBER_DECIMAL,
  TOKEN_TYPE_NUMBER_HEX,
  TOKEN_TYPE_NUMBER_BINARY,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_OPERATOR,
  TOKEN_TYPE_IDENTIFIER,
} TokenType;

typedef struct {
  TokenType type;
  String* source;
  String* file;
  size_t line;
  size_t pos;
} Token;

typedef struct {
  size_t length;
  Token* tokens;
  String* lines;
} TokenList;


// @Precondition: file data is never freed.
// @Precondition: input data is never freed.
TokenList* tokenize_string(String* file, String* input) {
  // @Lazy: We're allocating as much memory as we could ever possibly need, in
  // the hopes that we can avoid ever having to reallocate mid-routine.  This is
  // likely *way* more memory than we actually need, but it's at least cheap and
  // fairly reliable.
  size_t input_length = input->length;
  Token* tokens = calloc(input_length + 1, sizeof(Token));
  String* lines = calloc(input_length, sizeof(String));
  size_t token_idx = 0;

  TokenType token_type = 0;
  size_t token_start = 0; // Position in the file where the current token began.
  size_t file_pos = 0;    // Position in the file we're currently parsing.

  size_t line_no = 0;     // Line number we're parsing.
  size_t _line_no;
  size_t line_pos = 0;    // Position within the line we're parsing.
  size_t line_start = 0;  // File offset for beginning of the current line.

  #define THIS  (input->data[file_pos])
  #define LAST  (input->data[file_pos - 1])
  #define NEXT  (file_pos + 1 < input->length ? input->data[file_pos + 1] : '\0')
  #define LENGTH  (file_pos - token_start)
  #define SOURCE  (substring(input, token_start, LENGTH))

  #define IS_WHITESPACE(T)    (T == ' ' || T == '\t')
  #define IS_NEWLINE(T)       (T == '\n')
  #define IS_BINARY_DIGIT(T)  (T == '_' || T == '0' || T == '1')
  #define IS_DECIMAL_DIGIT(T) (T == '_' || (T >= '0' && T <= '9'))
  #define IS_HEX_DIGIT(T)     (IS_DECIMAL_DIGIT(T) || (T >= 'a' && T <= 'z') || (T >= 'A' && T <= 'Z'))
  #define IS_OPERATOR(T)      (T == '!' || T == '`' || (T >= '#' && T <= '/') || (T >= ':' && T <= '@') || (T >= '[' && T <= '^') || (T >= '{' && T <= '~'))
  #define IS_IDENTIFIER(T)    (!(IS_WHITESPACE(T) || IS_NEWLINE(T) || IS_OPERATOR(T)))

  #define ADVANCE(EXPECTED)   do { assert(EXPECTED == THIS); file_pos += 1; line_pos += 1; } while (0)

  #define SLURP(COND)             while (file_pos < input_length && COND) { ADVANCE(THIS); }
  #define SLURP_WHITESPACE()      SLURP(IS_WHITESPACE(THIS))
  #define SLURP_NEWLINES()        SLURP(IS_NEWLINE(THIS))
  #define SLURP_TO_EOL()          SLURP(!IS_NEWLINE(THIS))
  #define SLURP_COMMENT()         SLURP(IS_NEWLINE(THIS))
  #define SLURP_BINARY_NUMBER()   SLURP(IS_BINARY_DIGIT(THIS))
  #define SLURP_DECIMAL_NUMBER()  SLURP(IS_DECIMAL_DIGIT(THIS))
  #define SLURP_HEX_NUMBER()      SLURP(IS_HEX_DIGIT(THIS))
  #define SLURP_OPERATOR()        SLURP(IS_OPERATOR(THIS))
  #define SLURP_IDENT()           SLURP(IS_IDENTIFIER(THIS))

  #define START()    (token_start = file_pos)
  #define COMMIT(T)  (tokens[token_idx++] = (Token) { T, SOURCE, file, line_no, line_pos - LENGTH })

  while (file_pos < input_length) {
    START();

    switch (THIS) {
      case ' ':
      case '\t':
        SLURP_WHITESPACE();
        COMMIT(TOKEN_TYPE_WHITESPACE);
        break;
      case '\n':
        _line_no = line_no;
        while (file_pos < input_length && IS_NEWLINE(THIS)) {
          lines[_line_no++] = *substring(input, line_start, file_pos - line_start);
          line_start = file_pos + 1;
          ADVANCE(THIS);
        }

        COMMIT(TOKEN_TYPE_NEWLINE);

        line_no = _line_no;
        line_pos = 0;
        break;
      case '"':
        do {
          ADVANCE('"');
          SLURP(THIS != '"' && !IS_NEWLINE(THIS));
        } while (LAST == '\\' && !IS_NEWLINE(THIS));

        if (file_pos < input_length && !IS_NEWLINE(THIS)) {
          ADVANCE('"');
        }

        COMMIT(TOKEN_TYPE_STRING);
        break;
      case '0'...'9':
        token_type = TOKEN_TYPE_NUMBER_DECIMAL;

        if (THIS == '0') {
          ADVANCE('0');
          if (THIS == 'x') {
            token_type = TOKEN_TYPE_NUMBER_HEX;
            ADVANCE('x');
            SLURP_HEX_NUMBER();
          } else if (THIS == 'b') {
            token_type = TOKEN_TYPE_NUMBER_BINARY;
            ADVANCE('b');
            SLURP_BINARY_NUMBER();
          } else {
            SLURP_DECIMAL_NUMBER();
          }
        } else {
          SLURP_DECIMAL_NUMBER();
        }

        COMMIT(token_type);
        break;
      case ',':
        ADVANCE(',');
        COMMIT(TOKEN_TYPE_OPERATOR);
        break;
      case '/':
        if (NEXT == '/') {
          SLURP_TO_EOL();
          COMMIT(TOKEN_TYPE_COMMENT);
          break;
        } else {
          // Continue to process as an operator.
        }
      case '!':
      case '`':
      case '#'...'+':
      case '-'...'.':
      case ':'...'@':
      case '['...'^':
      case '{'...'~':
        SLURP_OPERATOR();
        COMMIT(TOKEN_TYPE_OPERATOR);
        break;
      default:
        SLURP_IDENT();
        COMMIT(TOKEN_TYPE_IDENTIFIER);
    }
  }

  TokenList* list = malloc(sizeof(TokenList));
  if (token_idx) {
    // if (tokens[token_idx - 1].type != TOKEN_TYPE_NEWLINE) {
    //   tokens[token_idx] = (Token) { TOKEN_TYPE_NEWLINE, new_string("\n"), file, line_no, line_pos + 1 };
    //   token_idx += 1;
    // }
    list->length = token_idx;
    list->tokens = realloc(tokens, list->length * sizeof(Token));
    list->lines = realloc(lines, list->length * sizeof(String));
  } else {
    list->length = 0;
    list->tokens = NULL;
    list->lines = NULL;
    free(tokens);
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
  #undef COMMIT

  return list;
}

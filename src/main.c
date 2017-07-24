#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ** String ** //

typedef struct {
  size_t length;
  char* data;
} String;

char* to_zero_terminated_string(String* str) {
  char* buffer = calloc(str->length, sizeof(char));

  memcpy(buffer, str->data, str->length);
  buffer[str->length] = '\0';

  return buffer;
}

String* substring(String* str, size_t pos, size_t length) {
  String* substr = malloc(sizeof(String));

  substr->length = length;
  substr->data = str->data + pos;

  return substr;
}


// ** File ** //

String* read_whole_file(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f) {
    return NULL;
  }

  struct stat s;
  fstat(fileno(f), &s);

  String* str = (String*) malloc(sizeof(String));
  str->length = s.st_size;
  str->data = malloc(s.st_size * sizeof(char));

  fread(str->data, s.st_size, 1, f);
  fclose(f);

  return str;
}


// ** Lexer ** //

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
  String* filename;
  size_t line;
  size_t pos;
} Token;

typedef struct {
  size_t length;
  Token* tokens;
} TokenList;


// @Precondition: filename data is never freed.
// @Precondition: input data is never freed.
// @Test: Is there a bug if the input string contains no tokens?
TokenList* tokenize_string(String* filename, String* input) {
  // @Lazy: We're allocating as much memory as we could ever possibly need, in
  // the hopes that we can avoid ever having to reallocate mid-routine.  This is
  // likely *way* more memory than we actually need, but it's at least cheap and
  // fairly reliable.
  size_t input_length = input->length;
  Token* tokens = calloc(input_length, sizeof(Token));
  size_t token_idx = 0;

  TokenType token_type = 0;
  size_t token_start = 0; // Position in the file where the current token began.
  size_t file_pos = 0;    // Position in the file we're currently parsing.

  size_t line_no = 0;  // Line number we're parsing.
  size_t line_pos = 0; // Position within the line we're parsing.

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

  #define ADVANCE()  do { file_pos += 1; line_pos += 1; if (file_pos >= input_length) break; } while (0)

  #define SLURP_WHITESPACE()      while (IS_WHITESPACE(THIS)) { ADVANCE(); }
  #define SLURP_NEWLINES()        while (IS_NEWLINE(THIS)) { ADVANCE(); }
  #define SLURP_TO_EOL()          while (!IS_NEWLINE(THIS)) { ADVANCE(); }
  #define SLURP_COMMENT()         while (IS_NEWLINE(THIS)) { ADVANCE(); }
  #define SLURP_BINARY_NUMBER()   while (IS_BINARY_DIGIT(THIS)) { ADVANCE(); }
  #define SLURP_DECIMAL_NUMBER()  while (IS_DECIMAL_DIGIT(THIS)) { ADVANCE(); }
  #define SLURP_HEX_NUMBER()      while (IS_HEX_DIGIT(THIS)) { ADVANCE(); }
  #define SLURP_OPERATOR()        while (IS_OPERATOR(THIS)) { ADVANCE(); }
  #define SLURP_STRING()          do { do { ADVANCE(); } while (LAST == '\\' || THIS != '"'); ADVANCE(); } while (0)
  #define SLURP_IDENT()           while (IS_IDENTIFIER(THIS)) { ADVANCE(); }

  #define START()    (token_start = file_pos)
  #define COMMIT(T)  (tokens[token_idx++] = (Token) { T, SOURCE, filename, line_no, line_pos - LENGTH })

  while (file_pos < input_length) {
    START();

    switch (THIS) {
      case ' ':
      case '\t':
        SLURP_WHITESPACE();
        COMMIT(TOKEN_TYPE_WHITESPACE);
        break;
      case '\n':
        SLURP_NEWLINES();
        COMMIT(TOKEN_TYPE_NEWLINE);
        line_no += 1;
        line_pos = 0;
        break;
      case '"':
        SLURP_STRING();
        COMMIT(TOKEN_TYPE_STRING);
        break;
      case '0'...'9':
        token_type = TOKEN_TYPE_NUMBER_DECIMAL;

        if (THIS == '0') {
          if (NEXT == 'x') {
            token_type = TOKEN_TYPE_NUMBER_HEX;
            ADVANCE();
            ADVANCE();
            SLURP_HEX_NUMBER();
          } else if (NEXT == 'b') {
            token_type = TOKEN_TYPE_NUMBER_BINARY;
            ADVANCE();
            ADVANCE();
            SLURP_BINARY_NUMBER();
          } else {
            SLURP_DECIMAL_NUMBER();
          }
        } else {
          SLURP_DECIMAL_NUMBER();
        }

        COMMIT(token_type);
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
      case '#'...'.':
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
  #undef SLURP_WHITESPACE
  #undef SLURP_NEWLINES
  #undef SLURP_TO_EOL
  #undef SLURP_COMMENT
  #undef SLURP_BINARY_NUMBER
  #undef SLURP_DECIMAL_NUMBER
  #undef SLURP_HEX_NUMBER
  #undef SLURP_OPERATOR
  #undef SLURP_STRING
  #undef SLURP_IDENT
  #undef START
  #undef COMMIT

  TokenList* list = malloc(sizeof(TokenList));
  if (token_idx) {
    list->length = token_idx;
    list->tokens = realloc(tokens, list->length * sizeof(Token));
  } else {
    list->length = 0;
    list->tokens = NULL;
    free(tokens);
  }

  return list;
}


// ** main ** //

#ifndef TESTING
int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>...\n", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    String filename = {
      .length = strlen(argv[i]), .data = (uint8_t*) argv[i],
    };

    String* result = read_whole_file(argv[i]);

    if (result != NULL) {
      tokenize_string(&filename, result);
    }
  }

  return 0;
}
#endif

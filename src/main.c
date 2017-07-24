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

  #define TOKEN_THIS  (input->data[file_pos])
  #define TOKEN_LAST  (input->data[file_pos - 1])
  #define TOKEN_NEXT  (file_pos + 1 < input->length ? input->data[file_pos + 1] : '\0')
  #define TOKEN_LENGTH  (file_pos - token_start)
  #define TOKEN_SOURCE  (substring(input, token_start, TOKEN_LENGTH))

  #define TOKEN_IS_WHITESPACE(T)    (T == ' ' || T == '\t')
  #define TOKEN_IS_NEWLINE(T)       (T == '\n')
  #define TOKEN_IS_BINARY_DIGIT(T)  (T == '_' || T == '0' || T == '1')
  #define TOKEN_IS_DECIMAL_DIGIT(T) (T == '_' || (T >= '0' && T <= '9'))
  #define TOKEN_IS_HEX_DIGIT(T)     (TOKEN_IS_DECIMAL_DIGIT(T) || (T >= 'a' && T <= 'z') || (T >= 'A' && T <= 'Z'))
  #define TOKEN_IS_OPERATOR(T)      (T == '!' || T == '`' || (T >= '#' && T <= '/') || (T >= ':' && T <= '@') || (T >= '[' && T <= '^') || (T >= '{' && T <= '~'))
  #define TOKEN_IS_IDENTIFIER(T)    (!(TOKEN_IS_WHITESPACE(T) || TOKEN_IS_NEWLINE(T) || TOKEN_IS_OPERATOR(T)))

  #define TOKEN_ADVANCE()  do { file_pos += 1; line_pos += 1; if (file_pos >= input_length) break; } while (0)

  #define TOKEN_SLURP_WHITESPACE()      while (TOKEN_IS_WHITESPACE(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_NEWLINES()        while (TOKEN_IS_NEWLINE(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_TO_EOL()          while (!TOKEN_IS_NEWLINE(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_COMMENT()         while (TOKEN_IS_NEWLINE(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_BINARY_NUMBER()   while (TOKEN_IS_BINARY_DIGIT(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_DECIMAL_NUMBER()  while (TOKEN_IS_DECIMAL_DIGIT(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_HEX_NUMBER()      while (TOKEN_IS_HEX_DIGIT(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_OPERATOR()        while (TOKEN_IS_OPERATOR(TOKEN_THIS)) { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_STRING()          do { do { TOKEN_ADVANCE(); } while (TOKEN_LAST == '\\' || TOKEN_THIS != '"'); TOKEN_ADVANCE(); } while (0)
  #define TOKEN_SLURP_IDENT()           while (TOKEN_IS_IDENTIFIER(TOKEN_THIS)) { TOKEN_ADVANCE(); }

  #define TOKEN_START()    (token_start = file_pos)
  #define TOKEN_COMMIT(T)  (tokens[token_idx++] = (Token) { T, TOKEN_SOURCE, filename, line_no, line_pos - TOKEN_LENGTH })

  while (file_pos < input_length) {
    switch (TOKEN_THIS) {
      case ' ':
      case '\t':
        TOKEN_START();
        TOKEN_SLURP_WHITESPACE();
        TOKEN_COMMIT(TOKEN_TYPE_WHITESPACE);
        break;
      case '\n':
        TOKEN_START();
        TOKEN_SLURP_NEWLINES();
        TOKEN_COMMIT(TOKEN_TYPE_NEWLINE);
        line_no += 1;
        line_pos = 0;
        break;
      case '"':
        TOKEN_START();
        TOKEN_SLURP_STRING();
        TOKEN_COMMIT(TOKEN_TYPE_STRING);
        break;
      case '0'...'9':
        token_type = TOKEN_TYPE_NUMBER_DECIMAL;

        TOKEN_START();

        if (TOKEN_THIS == '0') {
          if (TOKEN_NEXT == 'x') {
            token_type = TOKEN_TYPE_NUMBER_HEX;
            TOKEN_ADVANCE();
            TOKEN_ADVANCE();
            TOKEN_SLURP_HEX_NUMBER();
          } else if (TOKEN_NEXT == 'b') {
            token_type = TOKEN_TYPE_NUMBER_BINARY;
            TOKEN_ADVANCE();
            TOKEN_ADVANCE();
            TOKEN_SLURP_BINARY_NUMBER();
          } else {
            TOKEN_SLURP_DECIMAL_NUMBER();
          }
        } else {
          TOKEN_SLURP_DECIMAL_NUMBER();
        }

        TOKEN_COMMIT(token_type);
        break;
      case '/':
        if (TOKEN_NEXT == '/') {
          TOKEN_START();
          TOKEN_SLURP_TO_EOL();
          TOKEN_COMMIT(TOKEN_TYPE_COMMENT);
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
        TOKEN_START();
        TOKEN_SLURP_OPERATOR();
        TOKEN_COMMIT(TOKEN_TYPE_OPERATOR);
        break;
      default:
        TOKEN_START();
        TOKEN_SLURP_IDENT();
        TOKEN_COMMIT(TOKEN_TYPE_IDENTIFIER);
    }
  }

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ** String ** //

typedef struct {
  usize_t length;
  char* data;
} String;

char* to_zero_terminated_string(String* str) {
  char* buffer = calloc(str->length, sizeof(char));

  memcpy(buffer, str->data, str->length);
  buffer[str->length] = '\0';

  return buffer;
}

String* substring(String* str, usize_t pos, usize_t length) {
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
  TOKEN_TYPE_NUMBER_DECIMAL,
  TOKEN_TYPE_NUMBER_HEX,
  TOKEN_TYPE_NUMBER_BINARY,
} TokenType;

typedef struct {
  TokenType type;
  String* source;
  String* filename;
  usize_t line;
  usize_t pos;
} Token;

typedef struct {
  usize_t length;
  Token* tokens;
} TokenList;


// void parser_finish_token(ParserState* state) {
//   state->tokens[state->token_idx] = (Token) {
//     .source = substring(state->source, state->token_start, state->file_pos - state->token_start),
//     .filename = state->filename,
//     .line = state->line_no,
//     .pos = state->line_pos,
//   };
//   state->token_idx += 1;
//   state->mode = WAITING_FOR_NEXT_TOKEN;
// }



// @Precondition: filename data is never freed.
// @Precondition: input data is never freed.
// @Test: Is there a bug if the input string contains no tokens?
TokenList* tokenize_string(String* filename, String* input) {
  // @Lazy: We're allocating as much memory as we could ever possibly need, in
  // the hopes that we can avoid ever having to reallocate mid-routine.  This is
  // likely *way* more memory than we actually need, but it's at least cheap and
  // fairly reliable.
  usize_t input_length = input->length;
  Token* tokens = calloc(input_length, sizeof(Token));
  usize_t token_idx = 0;

  TokenType token_type = 0;
  usize_t token_start = 0; // Position in the file where the current token began.
  usize_t file_pos = 0;    // Position in the file we're currently parsing.

  usize_t line_no = 0;  // Line number we're parsing.
  usize_t line_pos = 0; // Position within the line we're parsing.

  #define TOKEN_THIS  (input->data[file_pos])
  #define TOKEN_LAST  (input->data[file_pos - 1])
  #define TOKEN_ADVANCE()  do { file_pos += 1; line_pos += 1; } while (0)
  #define TOKEN_NEWLINE()  do { TOKEN_START(); TOKEN_SLURP_NEWLINES(); TOKEN_COMMIT(TOKEN_TYPE_NEWLINE); line_no += 1; line_pos = 0;  } while (0)
  #define TOKEN_START()  (token_start = file_pos)
  #define TOKEN_SLURP_WHITESPACE()  while (TOKEN_THIS == ' ' || TOKEN_THIS == '\t') { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_NEWLINES()  while (TOKEN_THIS == '\n') { TOKEN_ADVANCE(); }
  #define TOKEN_SLURP_NUMBER()  while (TOKEN_THIS == '0' || TOKEN_THIS == '1' || TOKEN_THIS == '2' || TOKEN_THIS == '3' || TOKEN_THIS == '4' || TOKEN_THIS == '5' || TOKEN_THIS == '6' || TOKEN_THIS == '7' || TOKEN_THIS == '8' || TOKEN_THIS == '9' || TOKEN_THIS == '_') { TOKEN_ADVANCE(); }
  #define TOKEN_SOURCE()  (substring(input, token_start, file_pos - token_start))
  #define TOKEN_COMMIT(TYPE)  (tokens[token_idx++] = (Token) { TYPE, TOKEN_SOURCE(), filename, line_no, line_pos - (file_pos - token_start) })

  while (file_pos < input_length) {
    switch (TOKEN_THIS) {
      case ' ':
      case '\t':
        TOKEN_START();
        TOKEN_SLURP_WHITESPACE();
        TOKEN_COMMIT(TOKEN_TYPE_WHITESPACE);
        break;
      case '\n':
        TOKEN_NEWLINE();
        break;
      case '0'...'9':
        token_type = TOKEN_TYPE_NUMBER_DECIMAL;

        TOKEN_START();
        TOKEN_ADVANCE();

        if (TOKEN_LAST == '0') {
          if (TOKEN_THIS == 'x') {
            token_type = TOKEN_TYPE_NUMBER_HEX;
            TOKEN_ADVANCE();
          } else if (TOKEN_THIS == 'b') {
            token_type = TOKEN_TYPE_NUMBER_BINARY;
            TOKEN_ADVANCE();
          }
        }

        TOKEN_SLURP_NUMBER();
        TOKEN_COMMIT(token_type);
        break;
      default:
        TOKEN_ADVANCE();
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
      // printf("%s", to_zero_terminated_string(result));

      tokenize_string(&filename, result);
    }
  }

  return 0;
}
#endif

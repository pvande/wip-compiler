#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "src/string.c"
#include "src/file.c"
#include "src/table.c"
#include "src/list.c"

typedef enum {
  TOKEN_UNKNOWN,
  TOKEN_DIRECTIVE,
  TOKEN_WHITESPACE,
  TOKEN_NEWLINE,
  TOKEN_COMMENT,
  TOKEN_NUMBER_DECIMAL,
  TOKEN_NUMBER_FRACTIONAL,
  TOKEN_NUMBER_HEX,
  TOKEN_NUMBER_BINARY,
  TOKEN_STRING,
  TOKEN_OPERATOR,
  TOKEN_IDENTIFIER,
} TokenType;

typedef enum {
  EXPR_IDENT,
} ExpressionType;

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


typedef struct {
  ExpressionType type;
} Expression;

typedef struct {
  Expression _;
  Token* identifier;
} ExpressionIdentifier;

typedef struct {
  Expression _;
  Token* literal;
} ExpressionLiteral;

typedef struct {
  Token* name;
  Token* type;
  Expression* value;
} Declaration;

typedef struct ParserScope {
  Table* declarations;

  struct ParserScope* parent_scope;
} ParserScope;

typedef struct {
  Token* tokens;
  String* lines;
  size_t length;
  size_t pos;

  ParserScope* current_scope;
} ParserState;

#include "src/debug.c"

#include "src/tokenizer.c"
#include "src/parser.c"

#ifndef TESTING

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>...\n", argv[0]);
    return 1;
  }

  ParserScope* scope = new_parser_scope();

  for (int i = 1; i < argc; i++) {
    printf("\e[1;37mReading %s...\e[0m\n", argv[i]);
    String* file = new_string(argv[i]);
    String* result = read_whole_file(argv[i]);

    if (result != NULL) {
      printf("\e[1;37mTokenizing...\e[0m\n");
      TokenList* list = tokenize_string(file, result);

      printf("\e[1;37mParsing...\e[0m\n");
      parse_tokens(list, scope);
    }
  }

  printf("\e[1;37mPrinting global scope...\e[0m\n");
  print_scope(scope);
  printf("\n");

  return 0;
}

#endif

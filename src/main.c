#define HERE  (printf("Got to %s line %d\n", __FILE__, __LINE__));

#define ____  ,
#define DEFINE(TYPE, NAME, VAL)  static TYPE _ ## NAME = (TYPE) VAL; TYPE* const NAME = &_ ## NAME;
#define DEFINE_STR(NAME, VAL)    DEFINE(String, NAME, { strlen(VAL) ____ VAL })

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
#include "src/queue.c"


typedef enum {
  TOKEN_UNKNOWN,
  TOKEN_DIRECTIVE,
  TOKEN_TAG,
  TOKEN_NEWLINE,
  TOKEN_NUMBER_DECIMAL,
  TOKEN_NUMBER_FRACTIONAL,
  TOKEN_NUMBER_HEX,
  TOKEN_NUMBER_BINARY,
  TOKEN_STRING,
  TOKEN_SYNTAX_OPERATOR,
  TOKEN_OPERATOR,
  TOKEN_IDENTIFIER,
} TokenType;

typedef struct {
  TokenType type;
  String* source;
  String* file;
  size_t line;
  size_t pos;
} Token;

typedef struct {
  Token* tokens;
  String* lines;
  size_t length;
} TokenList;


typedef struct ParserScope {
  Table* declarations;

  struct ParserScope* parent_scope;
} ParserScope;

typedef struct {
  TokenList list;
  size_t pos;

  ParserScope* current_scope;
} ParserState;


typedef enum {
  EXPR_LITERAL,
  EXPR_IDENT,
  EXPR_UNPARSED_FUNCTION,
  EXPR_FUNCTION,
  EXPR_UNARY_OP,
  EXPR_BINARY_OP,
} ExpressionType;

typedef struct {
  ExpressionType type;
} Expression;

typedef struct {
  Expression base;
  Token* identifier;
} IdentifierExpression;

typedef struct {
  Expression base;
  Token* literal;
} LiteralExpression;

typedef struct {
  Expression base;
  ParserScope* scope;
  List* arguments;
  List* returns;
  List* body;
} FunctionExpression;

typedef struct {
  Expression base;
  Token* operator;
  Expression* rhs;
} UnaryOpExpression;

typedef struct {
  Expression base;
  Token* operator;
  Expression* lhs;
  Expression* rhs;
} BinaryOpExpression;


typedef struct {
  Token* name;
  Token* type;
  Expression* value;
} Declaration;


typedef enum {
  JOB_SENTINEL,
  JOB_READ,
  JOB_LEX,
  JOB_PARSE_FILE,
} JobType;

typedef struct {
  JobType type;
} Job;

typedef struct {
  Job base;
  String* filename;
} ReadJob;

typedef struct {
  Job base;
  String* filename;
  String* source;
} LexJob;

typedef struct {
  Job base;
  String* filename;
  TokenList* tokens;
} FileParseJob;

#include "src/debug.c"

#include "src/pipeline.c"
#include "src/tokenizer.c"
#include "src/parser.c"

#ifndef TESTING

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  initialize_pipeline();
  pipeline_emit_read_job(new_string(argv[1]));

  int did_work = 1;
  while (pipeline_has_jobs()) {
    Job* _job = pipeline_take_job();

    if (_job->type == JOB_READ) {
      ReadJob* job = (ReadJob*) _job;

      // @Lazy `filename.data` may not be naturally zero-terminated.
      char* file = to_zero_terminated_string(job->filename);
      String* source = file_read_all(file);
      free(file);

      if (source == NULL) {
        // @TODO: Record an error about not being able to find this file.
        free(job);
        continue;
      }

      pipeline_emit_lex_job(job->filename, source);
      did_work = 1;

    } else if (_job->type == JOB_LEX) {
      LexJob* job = (LexJob*) _job;

      TokenList* tokens = tokenize_string(job->filename, job->source);
      pipeline_emit_file_parse_job(job->filename, tokens);
      did_work = 1;

    } else if (_job->type == JOB_PARSE_FILE) {
      FileParseJob* job = (FileParseJob*) _job;

      ParserScope* file_scope = calloc(1, sizeof(ParserScope));
      file_scope->declarations = new_table(128);

      parse_file(job->tokens, file_scope);

      print_scope(file_scope);
      did_work = 1;

    } else if (_job->type == JOB_SENTINEL) {
      if (!did_work) break;
      did_work = 0;
      pipeline_emit(_job);
      continue;
    }

    free(_job);
  }

  printf("\n");

  return 0;
}

#endif

#define HERE  (printf("Got to %s line %d\n", __FILE__, __LINE__));

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


typedef struct ParserScope {
  Table* declarations;

  struct ParserScope* parent_scope;
} ParserScope;

typedef enum {
  TOKEN_UNKNOWN,
  TOKEN_DIRECTIVE,
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
  EXPR_LITERAL,
  EXPR_IDENT,
  EXPR_FUNCTION,
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
  Expression base;
  Token* identifier;
} IdentifierExpression;

typedef struct {
  Expression base;
  Token* literal;
} LiteralExpression;

typedef struct {
  Expression base;
  List* args;
  ParserScope* scope;
  List* instructions;
} FunctionExpression;

typedef struct {
  Token* name;
  Token* type;
  Expression* value;
} Declaration;

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

typedef enum {
  JOB_SENTINEL,
  JOB_READ,
  JOB_LEX,
  JOB_PARSE,
} JobType;

typedef struct {
  JobType type;
} Job;

typedef struct {
  Job base;
  String filename;
} ReadJob;

typedef struct {
  Job base;
  String filename;
  String source;
} LexJob;

typedef struct {
  Job base;
  TokenList* tokens;
} ParseJob;

Queue JOB_QUEUE;
ParserScope* GLOBAL_SCOPE;

void process_queue() {
  int did_work = 0;

  while (queue_length(&JOB_QUEUE) > 0) {
    Job* _job = queue_pull(&JOB_QUEUE);

    if (_job->type == JOB_READ) {
      ReadJob* job = (ReadJob*) _job;

      LexJob* next_job = malloc(sizeof(LexJob));
      next_job->base.type = JOB_LEX;
      next_job->filename = job->filename;

      {
        // @Lazy `filename.data` will not be zero-terminated if it came from a
        //       #load directive.  We could avoid the repeated heap allocations
        //       if we maintained a zeroed byte string we could copy the string
        //       data into.
        char* file = to_zero_terminated_string(&job->filename);

        // @Performance This has higher memory overhead than incrementally
        //              reading the file, but is faster overall.
        size_t result = file_read_all_into(file, &next_job->source);
        free(file);

        if (result == -1) {
          // @TODO: Record an error about not being able to find this file.
          free(next_job);
          free(job);
          continue;
        }
      }

      queue_add(&JOB_QUEUE, next_job);
      did_work = 1;

    } else if (_job->type == JOB_LEX) {
      LexJob* job = (LexJob*) _job;

      ParseJob* next_job = malloc(sizeof(ParseJob));
      next_job->base.type = JOB_PARSE;
      next_job->tokens = tokenize_string(&job->filename, &job->source);

      queue_add(&JOB_QUEUE, next_job);

      did_work = 1;

    } else if (_job->type == JOB_PARSE) {
      ParseJob* job = (ParseJob*) _job;

      parse_tokens(job->tokens, GLOBAL_SCOPE);

      did_work = 1;

    } else if (_job->type == JOB_SENTINEL) {
      if (!did_work) break;
      did_work = 0;

      queue_add(&JOB_QUEUE, _job);
      continue;
    }

    free(_job);
  }
}

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>...\n", argv[0]);
    return 1;
  }

  initialize_queue(&JOB_QUEUE, 1, 1);

  GLOBAL_SCOPE = new_parser_scope();

  for (int i = 1; i < argc; i++) {
    // @Lazy We should be able to get away with far fewer Job allocations if
    // we had a pool of Job descriptions for each type.
    ReadJob* job = malloc(sizeof(ReadJob));
    job->base.type = JOB_READ;
    job->filename.length = strlen(argv[i]);
    job->filename.data = argv[i];
    queue_add(&JOB_QUEUE, job);
  }

  // Since we constantly re-enqueue incomplete work, this sentinel value will
  // allow us to detect unsolvable cycles and abort cleanly.
  Job sentinel = (Job) { JOB_SENTINEL };
  queue_add(&JOB_QUEUE, &sentinel);

  process_queue();

  print_scope(GLOBAL_SCOPE);
  printf("\n");

  return 0;
}

#endif

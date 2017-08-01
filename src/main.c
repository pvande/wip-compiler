#define HERE  (printf("\nGot to %s line %d\n", __FILE__, __LINE__));

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
#include "src/symbol.c"

typedef char bool;

typedef enum {
  TOKEN_UNKNOWN,
  TOKEN_DIRECTIVE,
  TOKEN_TAG,
  TOKEN_NEWLINE,
  TOKEN_LITERAL,
  TOKEN_SYNTAX_OPERATOR,
  TOKEN_OPERATOR,
  TOKEN_IDENTIFIER,
} TokenType;

typedef enum {
  NONLITERAL      = 0,
  LITERAL_NUMBER  = (1 << 0),
  NUMBER_DECIMAL  = (1 << 1) + 1,
  NUMBER_HEX      = (1 << 2) + 1,
  NUMBER_BINARY   = (1 << 3) + 1,
  NUMBER_FRACTION = (1 << 4) + 1,

  LITERAL_STRING  = (1 << 5),
} TokenLiteralType;

typedef struct {
  TokenType type;
  String file;
  size_t line;
  size_t pos;

  String source;

  TokenLiteralType literal_type;
  char is_well_formed;
} Token;

typedef struct {
  String file;
  String* lines;

  Token* tokens;
  size_t count;
} TokenizedFile;


typedef struct {
  List* resolved_declarations;
  size_t declaration_count;
} CompilationWorkspace;

typedef struct ParserScope {
  List* declarations;

  struct ParserScope* parent_scope;
} ParserScope;

typedef struct {
  TokenizedFile list;
  size_t pos;

  ParserScope* current_scope;
} ParserState;


typedef enum {
  EXPR_LITERAL,
  EXPR_IDENT,
  EXPR_FUNCTION,
  EXPR_UNARY_OP,
  EXPR_BINARY_OP,
} ExpressionType;

typedef struct {
  ExpressionType type;
} AstExpression;

typedef struct {
  AstExpression base;
  Token* identifier;
} IdentifierExpression;

typedef struct {
  AstExpression base;
  Token* literal;
} LiteralExpression;

typedef struct {
  AstExpression base;
  String* name;
  ParserScope* scope;
  List* arguments;
  List* returns;
  List* body;
  List* instructions;
} FunctionExpression;

typedef struct {
  AstExpression base;
  Token* operator;
  AstExpression* rhs;
} UnaryOpExpression;

typedef struct {
  AstExpression base;
  Token* operator;
  AstExpression* lhs;
  AstExpression* rhs;
} BinaryOpExpression;


typedef struct {
  Token* type;
  Token* name;
} ReturnType;

typedef struct {
  Token* name;
  Token* type;
  AstExpression* value;
} AstDeclaration;


typedef enum {
  JOB_SENTINEL,
  JOB_READ,
  JOB_LEX,
  JOB_PARSE,
  JOB_TYPECHECK,
  JOB_OPTIMIZE,
  JOB_BYTECODE,
  JOB_OUTPUT,
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
  TokenizedFile* tokens;
} ParseJob;

typedef struct {
  Job base;
  AstDeclaration* declaration;
} TypecheckJob;

typedef struct {
  Job base;
  AstDeclaration* declaration;
} OptimizeJob;

typedef struct {
  Job base;
  AstDeclaration* declaration;
} BytecodeJob;

typedef struct {
  Job base;
} OutputJob;

#include "src/debug.c"

#include "src/pipeline.c"
#include "src/tokenizer.c"
#include "src/parser.c"
#include "src/bytecode.c"

#ifndef TESTING

#include <execinfo.h>
#include <signal.h>

void crashbar(int nSignum, siginfo_t* si, void* vcontext) {
  printf("\nSegmentation fault!\n");

  void* trace[10];
  size_t size = backtrace(trace, 10);
  backtrace_symbols_fd(trace, size, 0);

  exit(1);
}

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  struct sigaction action = {0};
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = crashbar;
  sigaction(SIGSEGV, &action, NULL);

  initialize_pipeline();
  pipeline_emit_read_job(new_string(argv[1]));

  CompilationWorkspace ws = { new_list(4, 64), 0 };

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

      TokenizedFile* tokens = tokenize_string(job->filename, job->source);
      pipeline_emit_parse_job(job->filename, tokens);
      did_work = 1;

    } else if (_job->type == JOB_PARSE) {
      ParseJob* job = (ParseJob*) _job;

      ParserScope* file_scope = calloc(1, sizeof(ParserScope));
      file_scope->declarations = new_list(4, 32);

      parse_file(job->tokens, file_scope);

      ws.declaration_count += file_scope->declarations->size;
      for (int i = 0; i < ws.declaration_count; i++) {
        AstDeclaration* decl = list_get(file_scope->declarations, i);
        pipeline_emit_typecheck_job(decl);
      }

      did_work = 1;

    } else if (_job->type == JOB_TYPECHECK) {
      TypecheckJob* job = (TypecheckJob*) _job;

      // @TODO Implement typechecking.
      {
        AstDeclaration* decl = job->declaration;
        if (decl->type == NULL) {
          decl->type = &(Token) { TOKEN_IDENTIFIER, *new_string(""), 0, 0, *new_string("void"), NONLITERAL, 1 };
        }
      }

      pipeline_emit_optimize_job(job->declaration);

      did_work = 1;

    } else if (_job->type == JOB_OPTIMIZE) {
      OptimizeJob* job = (OptimizeJob*) _job;

      // @TODO Implement optimizations.

      pipeline_emit_bytecode_job(job->declaration);

      did_work = 1;

    } else if (_job->type == JOB_BYTECODE) {
      BytecodeJob* job = (BytecodeJob*) _job;

      // @Lazy Make sure the declarations are passed in the job.
      // @TODO Implement optimizations.
      List* instructions = bytecode_generate(job->declaration);

      list_add(ws.resolved_declarations, job->declaration);

      if (ws.declaration_count == ws.resolved_declarations->size) {
        pipeline_emit_output_job(ws.resolved_declarations);
      }

      did_work = 1;

    } else if (_job->type == JOB_OUTPUT) {
      OutputJob* job = (OutputJob*) _job;

      // @TODO Refactor this into its own method.
      {
        // @TODO Inline base types and runtime support here.

        printf("\n");
        for (size_t i = 0; i < ws.declaration_count; i++) {
          AstDeclaration* decl = list_get(ws.resolved_declarations, i);

          printf("%s ·%s", to_zero_terminated_string(&decl->type->source),
                           to_zero_terminated_string(&decl->name->source));
          if (decl->value && decl->value->type == EXPR_FUNCTION) {
            printf("()");
          }
          printf(";  // Line %zu of ", decl->name->line + 1);
          print_string(&decl->name->file);
          printf("\n");
        }

        printf("\n\n");
        for (size_t i = 0; i < ws.declaration_count; i++) {
          AstDeclaration* decl = list_get(ws.resolved_declarations, i);

          printf("%s ·%s", to_zero_terminated_string(&decl->type->source),
                           to_zero_terminated_string(&decl->name->source));
          if (decl->value && decl->value->type == EXPR_FUNCTION) {
            printf("()");
          }
          printf(" {\n");
          printf("}\n");
        }

        printf("\n\n");
        printf("int main() { ·main(); }\n");
      }

      did_work = 1;

    } else if (_job->type == JOB_SENTINEL) {
      if (!did_work) break;
      did_work = 0;
      pipeline_emit(_job);
      continue;
    }

    free(_job);
  }

  return 0;
}

#endif

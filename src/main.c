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
#include "src/pool.c"
#include "src/queue.c"
#include "src/stack.c"
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
  String filename;
  String* lines;

  Token* tokens;
  size_t length;
} TokenizedFile;


typedef struct {
} CompilationWorkspace;


typedef enum {
  EXPR_LITERAL,
  EXPR_IDENT,
  EXPR_FUNCTION,
  EXPR_UNARY_OP,
  EXPR_BINARY_OP,
  EXPR_CALL,
} ExpressionType;

typedef enum {
  STATEMENT_ASSIGNMENT,
  STATEMENT_BLOCK,
  STATEMENT_CONDITIONAL,
  STATEMENT_DECLARATION,
  STATEMENT_EXPRESSION,
  STATEMENT_LOOP,
} StatementType;

typedef enum {
  NODE_ASSIGNMENT,
  NODE_BRANCH,
  NODE_COMPOUND,
  NODE_DECLARATION,
  NODE_EXPRESSION,
  NODE_FOREACH,
} AstNodeType;

typedef enum {
  NODE_IS_FUNCTION_DECLARATION = (1 << 0),
} AstNodeFlags;

typedef struct {
  size_t line;
  size_t pos;
} FileAddress;

typedef struct AstNode {
  AstNodeType node_type;
  AstNodeFlags flags;

  FileAddress from;
  FileAddress to;

  Symbol ident;
  struct AstNode* lhs;
  struct AstNode* rhs;

  String* error;
} AstNode;


/// -----------------
// typedef struct {
//   Symbol name;
// } AstType;
//
// typedef struct {
//   ExpressionType type;
// } AstExpression;
//
// typedef struct {
//   AstExpression base;
//   Token* identifier;
// } IdentifierExpression;
//
// typedef struct {
//   AstExpression base;
//   Token* literal;
// } LiteralExpression;
//
// typedef struct {
//   AstExpression base;
//   Symbol name;
//   ParserScope* scope;
//   List* arguments;
//   AstType returns;
//   List* body;
//   List* instructions;
// } FunctionExpression;
//
// typedef struct {
//   AstExpression base;
//   Token* operator;
//   AstExpression* rhs;
// } UnaryOpExpression;
//
// typedef struct {
//   AstExpression base;
//   Token* operator;
//   AstExpression* lhs;
//   AstExpression* rhs;
// } BinaryOpExpression;
//
// typedef struct {
//   AstExpression base;
//   Symbol function;
//   List* arguments;
// } CallExpression;
//
// typedef struct {
//   Symbol name;
//   AstType* type;
//   AstExpression* value;
// } AstDeclaration;
//
// typedef struct {
//   StatementType type;
//   void* data;
// } AstStatement;
/// -----------------

#include "src/debug.c"

#include "src/pipeline.c"

#include "src/reader.c"
#include "src/lexer.c"
#include "src/parser.c"
#include "src/bytecode.c"
#include "src/output.c"

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
  pipeline_emit_read_job((String) { strlen(argv[1]), argv[1] });

  int did_work = 1;
  while (pipeline_has_jobs()) {
    Job* job = pipeline_take_job();

    if (job->type == JOB_READ) {
      did_work = perform_read_job((ReadJob*) job);

    } else if (job->type == JOB_LEX) {
      did_work = perform_lex_job((LexJob*) job);

    } else if (job->type == JOB_PARSE) {
      did_work = perform_parse_job((ParseJob*) job);
    //   ParseJob* job = (ParseJob*) _job;
    //
    //   ParserScope* file_scope = calloc(1, sizeof(ParserScope));
    //   file_scope->declarations = new_list(4, 32);
    //
    //   parse_file(job->tokens, file_scope);
    //
    //   // ws.declaration_count += file_scope->declarations->size;
    //   // for (int i = 0; i < ws.declaration_count; i++) {
    //   //   AstDeclaration* decl = list_get(file_scope->declarations, i);
    //   //   pipeline_emit_typecheck_job(decl);
    //   // }
    //
    //   did_work = 1;

    // } else if (_job->type == JOB_TYPECHECK) {
    //   TypecheckJob* job = (TypecheckJob*) _job;
    //
    //   // @TODO Implement typechecking.
    //
    //   pipeline_emit_optimize_job(job->declaration);
    //
    //   did_work = 1;
    //
    // } else if (_job->type == JOB_OPTIMIZE) {
    //   OptimizeJob* job = (OptimizeJob*) _job;
    //
    //   // @TODO Implement optimizations.
    //
    //   pipeline_emit_bytecode_job(job->declaration);
    //
    //   did_work = 1;
    //
    // } else if (_job->type == JOB_BYTECODE) {
    //   BytecodeJob* job = (BytecodeJob*) _job;
    //
    //   // @Lazy Make sure the declarations are passed in the job.
    //   // @TODO Implement optimizations.
    //   List* instructions = bytecode_generate(job->declaration);
    //
    //   list_add(ws.resolved_declarations, job->declaration);
    //
    //   if (ws.declaration_count == ws.resolved_declarations->size) {
    //     pipeline_emit_output_job(ws.resolved_declarations);
    //   }
    //
    //   did_work = 1;
    //
    // } else if (_job->type == JOB_OUTPUT) {
    //   OutputJob* job = (OutputJob*) _job;
    //
    //   // @Lazy Make sure the declarations are passed in the job.
    //   // output_c_code_for_declarations(ws.resolved_declarations);
    //
    //   did_work = 1;

    } else if (job->type == JOB_SENTINEL) {
      if (!did_work) break;
      did_work = 0;
      pipeline_emit(job);
      continue;
    }

    free(job);
  }

  return 0;
}

#endif

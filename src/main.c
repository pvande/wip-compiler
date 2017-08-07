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

// ** Constant Strings ** //

DEFINE_STR(STR_VOID, "void");
DEFINE_STR(STR_BYTE, "byte");
DEFINE_STR(STR_U8,  "u8");
DEFINE_STR(STR_U16, "u16");
DEFINE_STR(STR_U32, "u32");
DEFINE_STR(STR_U64, "u64");
DEFINE_STR(STR_S8,  "s8");
DEFINE_STR(STR_S16, "s16");
DEFINE_STR(STR_S32, "s32");
DEFINE_STR(STR_S64, "s64");
DEFINE_STR(STR_INT, "int");

typedef char bool;

typedef enum {
  TOKEN_UNKNOWN,
  TOKEN_DIRECTIVE,
  TOKEN_TAG,
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

typedef struct Scope {
  struct Scope* parent;
  List* declarations;
} Scope;

// Update docs/parser/node-usage.md when this changes.
typedef enum {
  NODE_ASSIGNMENT,
  // NODE_BRANCH,
  NODE_COMPOUND,
  NODE_DECLARATION,
  NODE_EXPRESSION,
  // NODE_LOOP,
  NODE_RECOVERY,
  NODE_TYPE,
} AstNodeType;

// Update docs/parser/node-usage.md when this changes.
typedef enum {
  COMPOUND_CODE_BLOCK  = (1 << 0),
  COMPOUND_DECL_ASSIGN = (1 << 1),
  EXPR_LITERAL         = (1 << 0),
  EXPR_IDENT           = (1 << 1),
  EXPR_FUNCTION        = (1 << 2),
  EXPR_CALL            = (1 << 3),
  // EXPR_UNARY_OP  = (1 << 4),
  // EXPR_BINARY_OP = (1 << 5),
} AstNodeFlags;

typedef struct {
  size_t line;
  size_t pos;
} FileAddress;

typedef struct {
  size_t id;
  size_t size;
  String* name;
} Typeclass;

// Update docs/parser/node-usage.md when this changes.
typedef struct AstNode {
  AstNodeType type;
  AstNodeFlags flags;         // 0
  size_t id;                  // Serial number

  FileAddress from;           // ---
  FileAddress to;             // ---

  Symbol ident;               // ---
  String source;              // ---
  struct AstNode* lhs;        // ---
  struct AstNode* rhs;        // ---
  Typeclass* typeclass;       // NULL

  size_t body_length;         // ---
  struct AstNode* body;       // ---

  Scope* scope;               // ---

  String* error;              // NULL
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
#include "src/typechecker.c"
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

  // @TODO Make the pipeline a part of the CompilationWorkspace.
  initialize_pipeline();
  pipeline_emit_read_job((String) { strlen(argv[1]), argv[1] });

  initialize_typechecker();

  int did_work = 1;
  while (pipeline_has_jobs()) {
    Job* job = pipeline_take_job();

    if (job->type == JOB_READ) {
      did_work = perform_read_job((ReadJob*) job);

    } else if (job->type == JOB_LEX) {
      did_work = perform_lex_job((LexJob*) job);

    } else if (job->type == JOB_PARSE) {
      did_work = perform_parse_job((ParseJob*) job);

    } else if (job->type == JOB_TYPECHECK) {
      did_work = perform_typecheck_job((TypecheckJob*) job);

      if (!did_work) {
        pipeline_emit(job);
        continue;
      }

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

  if (pipeline_has_jobs()) {
    fprintf(stderr, "Unable to compile %s.\n", argv[1]);
    return 1;
  } else {
    fprintf(stderr, "Compiled %s.\n", argv[1]);
    return 0;
  }
}

#endif

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

DEFINE_STR(STR_VOID,   "void");
DEFINE_STR(STR_BOOL,   "bool");
DEFINE_STR(STR_BYTE,   "byte");
DEFINE_STR(STR_U8,     "u8");
DEFINE_STR(STR_U16,    "u16");
DEFINE_STR(STR_U32,    "u32");
DEFINE_STR(STR_U64,    "u64");
DEFINE_STR(STR_S8,     "s8");
DEFINE_STR(STR_S16,    "s16");
DEFINE_STR(STR_S32,    "s32");
DEFINE_STR(STR_S64,    "s64");
DEFINE_STR(STR_INT,    "int");
DEFINE_STR(STR_FLOAT,  "float");
DEFINE_STR(STR_STRING, "string");

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
  NONLITERAL             = 0,
  IS_DECIMAL_LITERAL     = (1 << 8),
  IS_HEX_LITERAL         = (1 << 9),
  IS_BINARY_LITERAL      = (1 << 10),
  IS_FRACTIONAL_LITERAL  = (1 << 11),
  IS_STRING_LITERAL      = (1 << 12),
} TokenLiteralType;

typedef struct {
  TokenType type;
  size_t line;
  size_t pos;

  String source;

  TokenLiteralType literal_type;
  bool is_well_formed;
} Token;


typedef struct {
  size_t line;
  size_t pos;
} FileAddress;

typedef struct {
  String* filename;
  String* source;
  String* lines;
  size_t length; // @TODO Rename `line_count`, or box `lines` in an "Array"
} FileInfo;

typedef struct {
  Token* tokens;
  size_t length;
} TokenizedFile;


typedef struct {
  Queue pipeline;
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
  EXPR_PROCEDURE       = (1 << 2),
  EXPR_CALL            = (1 << 3),
  // EXPR_UNARY_OP  = (1 << 4),
  // EXPR_BINARY_OP = (1 << 5),
  NODE_CONTAINS_LHS    = (1 << 29),
  NODE_CONTAINS_RHS    = (1 << 30),
  NODE_CONTAINS_ERROR  = (1 << 31),
} AstNodeFlags;

typedef struct {
  size_t id;
  size_t size;
  String* name;
  List* from;
  List* to;
} Typeclass;

typedef enum {
  KIND_NUMBER        = (1 << 0),
  KIND_CAN_BE_SIGNED = (1 << 1),
  KIND_CAN_BE_U8     = (1 << 2),
  KIND_CAN_BE_U16    = (1 << 3),
  KIND_CAN_BE_U32    = (1 << 4),
  KIND_CAN_BE_U64    = (1 << 5),
} Typekind;

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

  size_t body_length;         // ---
  struct AstNode* body;       // ---

  Scope* scope;               // ---

  Typeclass* typeclass;       // NULL
  Typekind typekind;          // 0
  union {
    unsigned long long int_value;
    double double_value;
    void* pointer_value;
  };

  String* error;              // NULL
} AstNode;

#include "src/debug.c"

#include "src/pipeline.c"

#include "src/reader.c"
#include "src/lexer.c"
#include "src/parser.c"
#include "src/typechecker.c"
#include "src/optimizer.c"
#include "src/bytecode.c"
#include "src/codegen.c"

DEFINE(Job, SENTINEL, { JOB_SENTINEL });
void initialize_workspace(CompilationWorkspace* ws) {
  initialize_queue(&ws->pipeline, 16, 16);

  // Since we constantly re-enqueue incomplete work, particularly during
  // typechecking, this sentinel value will allow us to detect unsolvable cycles
  // and abort cleanly.
  pipeline_emit(ws, SENTINEL);
}

void report_errors(FileInfo* file, AstNode* node) {
  if (!(node->flags & NODE_CONTAINS_ERROR)) return;

  if (node->error == NULL) {
    if (node->flags & NODE_CONTAINS_LHS) report_errors(file, node->lhs);
    if (node->flags & NODE_CONTAINS_RHS) report_errors(file, node->rhs);
    for (size_t i = 0; i < node->body_length; i++) report_errors(file, &node->body[i]);
  } else {
    size_t line_no = node->from.line;

    char* filename = to_zero_terminated_string(file->filename);
    String* line = &file->lines[line_no];
    char* line_str = to_zero_terminated_string(line);

    char* bold = "\e[1;37m";
    char* code = "\e[0;36m";
    char* err = "\e[0;31m";
    char* reset = "\e[0m";

    printf("Error: "); print_string(node->error); printf("\n");
    printf("In %s%s%s on line %s%zu%s\n\n", bold, filename, reset, bold, line_no + 1, reset);
    printf("> %s%s%s\n", code, line_str, reset);

    for (int i = 0; i < line->length; i++) line_str[i] = ' ';
    for (int i = node->from.pos; i < node->to.pos; i++) line_str[i] = '^';
    printf("  %s%s%s\n", err, line_str, reset);

    free(filename);
    free(line_str);
  }
}

bool begin_compilation(CompilationWorkspace* ws) {
  initialize_typechecker();  // @TODO Push this into the CompilationWorkspace.

  bool did_work = 1;
  int reported_errors = 0;
  while (pipeline_has_jobs(ws)) {
    Job* job = pipeline_take_job(ws);

    if (job->type == JOB_READ) {
      did_work |= perform_read_job(job);

    } else if (job->type == JOB_LEX) {
      did_work |= perform_lex_job(job);

    } else if (job->type == JOB_PARSE) {
      did_work |= perform_parse_job(job);

    } else if (job->type == JOB_TYPECHECK) {
      bool result = perform_typecheck_job(job);
      did_work |= result;

      if (!result) {
        pipeline_emit(ws, job);
        continue;
      }

    } else if (job->type == JOB_OPTIMIZE) {
      did_work |= perform_optimize_job(job);

    } else if (job->type == JOB_BYTECODE) {
      did_work |= perform_bytecode_job(job);

    } else if (job->type == JOB_ABORT) {
      report_errors(job->file, job->node);
      reported_errors += 1;
      did_work = 1;

    } else if (job->type == JOB_SENTINEL) {
      if (!did_work) break;
      did_work = 0;
      pipeline_emit(ws, job);
      continue;
    }

    free(job);
  }

  // Drain the remaining pipeline jobs for error reporting.
  while (pipeline_has_jobs(ws)) {
    Job* job = pipeline_take_job(ws);

    if (job->type == JOB_SENTINEL) assert(0);

    reported_errors += 1;
    if (job->type == JOB_TYPECHECK) {
      // printf("«««««««»»»»»»»\n");
      // print_ast_node_as_tree(job->file->lines, job->node);
      // printf("«««««««»»»»»»»\n");
      printf("\n\n");
      report_errors(job->file, job->node);

    } else {
      printf("Unknown job error type: %d\n", job->type);
      assert(0);
    }

    free(job);
  }

  if (reported_errors > 0) {
    return 0;
  } else {
    // @TODO Codegen.
    return 1;
  }
}

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

  CompilationWorkspace workspace;
  initialize_workspace(&workspace);

  pipeline_emit_read_job(&workspace, &(String) { strlen(argv[1]), argv[1] });

  if (begin_compilation(&workspace)) {
    fprintf(stderr, "Compiled %s.\n", argv[1]);
  } else {
    fprintf(stderr, "Unable to compile %s.\n", argv[1]);
  }
}

#endif

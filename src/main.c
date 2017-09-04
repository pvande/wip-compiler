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

DEFINE_STR(DEFAULT_ENTRY_POINT, "main");

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


typedef struct Scope {
  struct Scope* parent;
  List declarations;
} Scope;

typedef struct {
  Queue pipeline;
  Symbol entry;
  size_t entry_id;
  List bytecode;
  List initializers;
  Scope global_scope;
} CompilationWorkspace;

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
  NODE_INITIALIZING    = (1 << 25),
  NODE_INITIALIZED     = (1 << 26),
  NODE_CONTAINS_IDENT  = (1 << 27),
  NODE_CONTAINS_SOURCE = (1 << 28),
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
  size_t bytecode_id;         // Serial number

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
    struct AstNode* declaration;
  };

  String* error;              // NULL
} AstNode;


enum BytecodeInstructions {
  BC_EXIT,
  BC_LOAD,
  BC_STORE,
  BC_LOAD_ARG,
  BC_PUSH,
  BC_CALL,

  BC_PRINT,
};



typedef struct {
  size_t id;
  size_t ip;
  size_t sp;
  size_t fp;

  size_t stack[512];

  AstNode* waiting_on;
} VmState;

#include "src/debug.c"

#include "src/pipeline.c"

#include "src/reader.c"
#include "src/lexer.c"
#include "src/parser.c"
#include "src/typechecker.c"
#include "src/optimizer.c"
#include "src/bytecode.c"
#include "src/codegen.c"
#include "src/interpreter.c"

DEFINE_STR(TYPE_PROC_U8_TO_VOID, "(u8) => ()");
DEFINE_STR(BUILTIN_PUTC, "putc");
void populate_builtins(CompilationWorkspace* ws) {
  size_t builtin_node_id = -1;

  {
    // @Hack Automatic interpretation of the `main` method.
    size_t* main_bytecode = malloc(3 * sizeof(size_t));
    main_bytecode[0] = BC_CALL;
    main_bytecode[1] = 0;  // Filled in later!
    main_bytecode[2] = BC_EXIT;
    list_append(&ws->bytecode, main_bytecode);
  }

  {
    AstNode* putc_decl = calloc(1, sizeof(AstNode));
    AstNode* putc_expr = calloc(1, sizeof(AstNode));
    size_t* putc_bytecode = malloc(4 * sizeof(size_t));
    putc_bytecode[0] = BC_LOAD_ARG;
    putc_bytecode[1] = 0;
    putc_bytecode[2] = BC_PRINT;
    putc_bytecode[3] = BC_EXIT;

    putc_expr->id = builtin_node_id--;
    putc_expr->type = NODE_EXPRESSION;
    putc_expr->flags = EXPR_PROCEDURE;
    putc_expr->bytecode_id = list_append(&ws->bytecode, putc_bytecode);

    putc_decl->id = builtin_node_id--;
    putc_decl->type = NODE_DECLARATION;
    putc_decl->flags = NODE_INITIALIZED;
    putc_decl->ident = symbol_get(BUILTIN_PUTC);
    putc_decl->typeclass = _new_type(TYPE_PROC_U8_TO_VOID, 64);
    putc_decl->typeclass->from = new_list(1, 1);
    putc_decl->typeclass->to = new_list(1, 0);
    putc_decl->pointer_value = putc_expr;

    list_append(putc_decl->typeclass->from, _get_type(STR_U8));
    list_append(&ws->global_scope.declarations, putc_decl);
  }
}

DEFINE(Job, SENTINEL, { JOB_SENTINEL });
void initialize_workspace(CompilationWorkspace* ws) {
  initialize_queue(&ws->pipeline, 16, 16);
  initialize_list(&ws->bytecode, 16, 64);
  initialize_list(&ws->initializers, 16, 512);
  initialize_list(&ws->global_scope.declarations, 16, 512);

  populate_builtins(ws);

  // Since we constantly re-enqueue incomplete work, particularly during
  // typechecking, this sentinel value will allow us to detect unsolvable cycles
  // and abort cleanly.
  pipeline_emit(ws, SENTINEL);

  ws->entry = symbol_get(DEFAULT_ENTRY_POINT);
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

      if (result) {
        if (job->node->type == NODE_ASSIGNMENT && job->node->lhs->ident == job->ws->entry) {
          // @Lazy This assumes that the rhs is a procedure!
          // job->ws->entry_id = job->node->rhs->id;
          size_t* bootstrap_bytecode = list_get(&job->ws->bytecode, 0);
          *(bootstrap_bytecode + 1) = (size_t) job->node->lhs;

          // @Hack Automatically running "main" in the interpreter.
          VmState* state = malloc(sizeof(VmState));
          state->fp = -1;
          state->sp = -1;
          state->ip = 0;
          state->id = 0;
          state->waiting_on = job->node->lhs;
          pipeline_emit_execute_job(job->ws, state);
        }
      } else {
        pipeline_emit(ws, job);
        continue;
      }

    } else if (job->type == JOB_OPTIMIZE) {
      did_work |= perform_optimize_job(job);

    } else if (job->type == JOB_BYTECODE) {
      bool result = perform_bytecode_job(job);
      did_work |= result;

      if (!result) {
        pipeline_emit(ws, job);
        continue;
      }

    } else if (job->type == JOB_EXECUTE) {
      bool result = perform_execute_job(job);
      did_work |= result;

      if (!result) {
        pipeline_emit(ws, job);
        continue;
      }

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

    } else if (job->type == JOB_BYTECODE) {
      // printf("«««««««»»»»»»»\n");
      // print_ast_node_as_tree(job->file->lines, job->node);
      // printf("«««««««»»»»»»»\n");
      // printf("\n\n");
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

  CompilationWorkspace workspace = {};
  initialize_typechecker();  // @TODO Push this into the CompilationWorkspace.
  initialize_workspace(&workspace);

  pipeline_emit_read_job(&workspace, &(String) { strlen(argv[1]), argv[1] });

  if (begin_compilation(&workspace)) {
    fprintf(stderr, "Compiled %s.\n", argv[1]);
    return 0;
  } else {
    fprintf(stderr, "Unable to compile %s.\n", argv[1]);
    return 1;
  }
}

#endif

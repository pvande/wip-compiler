Queue JOB_QUEUE;

typedef enum {
  JOB_SENTINEL,
  JOB_READ,
  JOB_LEX,
  JOB_PARSE,
  JOB_TYPECHECK,
  JOB_OPTIMIZE,
  JOB_BYTECODE,
  JOB_ABORT,
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
  TokenizedFile* tokens;
  FileDebugInfo* debug;
} ParseJob;

typedef struct {
  Job base;
  AstNode* node;
  FileDebugInfo* debug;
} TypecheckJob;

typedef struct {
  Job base;
  AstNode* node;
  FileDebugInfo* debug;
} OptimizeJob;

typedef struct {
  Job base;
  AstNode* node;
} BytecodeJob;

typedef struct {
  Job base;
  AstNode* node;
  FileDebugInfo* debug;
} AbortJob;


void pipeline_emit(void* job) {
  queue_add(&JOB_QUEUE, job);
}

int pipeline_has_jobs() {
  return queue_length(&JOB_QUEUE) > 0;
}

Job* pipeline_take_job() {
  return queue_pull(&JOB_QUEUE);
}

void pipeline_emit_read_job(String filename) {
  // fprintf(stderr, "Emitting ReadJob for "); print_string(&filename); printf("\n");

  // @Lazy We should use a pool allocator.
  ReadJob* job = malloc(sizeof(ReadJob));
  job->base.type = JOB_READ;
  job->filename = filename;
  pipeline_emit(job);
}

void pipeline_emit_lex_job(String filename, String source) {
  // fprintf(stderr, "Emitting LexJob for %zu bytes of ", source.length); print_string(&filename); printf("\n");

  // @Lazy We should use a pool allocator.
  LexJob* job = malloc(sizeof(LexJob));
  job->base.type = JOB_LEX;
  job->filename = filename;
  job->source = source;

  pipeline_emit(job);
}

void pipeline_emit_parse_job(FileDebugInfo* debug, TokenizedFile* tokens) {
  // fprintf(stderr, "Emitting FileParseJob for %zu tokens of ", tokens->length); print_string(&tokens->filename); printf("\n");

  // @Lazy We should use a pool allocator.
  ParseJob* job = malloc(sizeof(ParseJob));
  job->base.type = JOB_PARSE;
  job->debug = debug;
  job->tokens = tokens;

  pipeline_emit(job);
}

void pipeline_emit_typecheck_job(FileDebugInfo* debug, AstNode* node) {
  // fprintf(stderr, "Emitting TypecheckJob for "); print_symbol(node->type == NODE_ASSIGNMENT ? node->lhs->ident : node->ident); printf("\n");

  // @Lazy We should use a pool allocator.
  TypecheckJob* job = malloc(sizeof(TypecheckJob));
  job->base.type = JOB_TYPECHECK;
  job->debug = debug;
  job->node = node;

  pipeline_emit(job);
}

void pipeline_emit_optimize_job(FileDebugInfo* debug, AstNode* node) {
  // fprintf(stderr, "Emitting OptimizeJob for "); print_symbol(decl->name);
  // printf(" in "); print_symbol(decl->name->file); printf(" line %zu\n", decl->name->line)

  // @Lazy We should use a pool allocator.
  OptimizeJob* job = malloc(sizeof(OptimizeJob));
  job->base.type = JOB_OPTIMIZE;
  job->debug = debug;
  job->node = node;

  pipeline_emit(job);
}

void pipeline_emit_bytecode_job(AstNode* node) {
  // fprintf(stderr, "Emitting BytecodeJob for "); print_symbol(decl->name);
  // printf(" in "); print_symbol(decl->name->file); printf(" line %zu\n", decl->name->line)

  // @Lazy We should use a pool allocator.
  BytecodeJob* job = malloc(sizeof(BytecodeJob));
  job->base.type = JOB_BYTECODE;
  job->node = node;

  pipeline_emit(job);
}

void pipeline_emit_abort_job(FileDebugInfo* debug, AstNode* node) {
  // @Lazy We should use a pool allocator.
  AbortJob* job = malloc(sizeof(TypecheckJob));
  job->base.type = JOB_ABORT;
  job->debug = debug;
  job->node = node;

  pipeline_emit(job);
}

DEFINE(Job, SENTINEL, { JOB_SENTINEL });
void initialize_pipeline() {
  initialize_queue(&JOB_QUEUE, 1, 1);

  // Since we constantly re-enqueue incomplete work, this sentinel value will
  // allow us to detect unsolvable cycles and abort cleanly.
  pipeline_emit(SENTINEL);
}

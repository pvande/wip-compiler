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
  CompilationWorkspace* ws;
  FileInfo* file;
  union {
    TokenizedFile* tokens;
    AstNode* node;
    String* source;
  };
} Job;


void pipeline_emit(CompilationWorkspace* ws, Job* job) {
  queue_add(&ws->pipeline, job);
}

int pipeline_has_jobs(CompilationWorkspace* ws) {
  return queue_length(&ws->pipeline) > 0;
}

Job* pipeline_take_job(CompilationWorkspace* ws) {
  return queue_pull(&ws->pipeline);
}

void pipeline_emit_read_job(CompilationWorkspace* ws, String* filename) {
  FileInfo* file = malloc(sizeof(FileInfo));
  file->filename = filename;

  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_READ;
  job->ws = ws;
  job->file = file;

  pipeline_emit(ws, job);
}

void pipeline_emit_lex_job(CompilationWorkspace* ws, FileInfo* file) {
  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_LEX;
  job->ws = ws;
  job->file = file;

  pipeline_emit(ws, job);
}

void pipeline_emit_parse_job(CompilationWorkspace* ws, FileInfo* file, TokenizedFile* tokens) {
  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_PARSE;
  job->ws = ws;
  job->file = file;
  job->tokens = tokens;

  pipeline_emit(ws, job);
}

void pipeline_emit_typecheck_job(CompilationWorkspace* ws, FileInfo* file, AstNode* node) {
  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_TYPECHECK;
  job->ws = ws;
  job->file = file;
  job->node = node;

  pipeline_emit(ws, job);
}

void pipeline_emit_optimize_job(CompilationWorkspace* ws, FileInfo* file, AstNode* node) {
  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_OPTIMIZE;
  job->ws = ws;
  job->file = file;
  job->node = node;

  pipeline_emit(ws, job);
}

void pipeline_emit_bytecode_job(CompilationWorkspace* ws, FileInfo* file, AstNode* node) {
  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_BYTECODE;
  job->ws = ws;
  job->file = file;
  job->node = node;

  pipeline_emit(ws, job);
}

void pipeline_emit_abort_job(CompilationWorkspace* ws, FileInfo* file, AstNode* node) {
  // @Lazy We should use a pool allocator.
  Job* job = malloc(sizeof(Job));
  job->type = JOB_ABORT;
  job->ws = ws;
  job->file = file;
  job->node = node;

  pipeline_emit(ws, job);
}

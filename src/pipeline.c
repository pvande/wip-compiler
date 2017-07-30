Queue JOB_QUEUE;

void pipeline_emit(void* job) {
  queue_add(&JOB_QUEUE, job);
}

int pipeline_has_jobs() {
  return queue_length(&JOB_QUEUE) > 0;
}

Job* pipeline_take_job() {
  return queue_pull(&JOB_QUEUE);
}

void pipeline_emit_read_job(String* filename) {
  printf("Emitting ReadJob for "); print_string(filename); printf("\n");

  // @Lazy We should use a pool allocator.
  ReadJob* job = malloc(sizeof(ReadJob));
  job->base.type = JOB_READ;
  job->filename = filename;
  pipeline_emit(job);
}

void pipeline_emit_lex_job(String* filename, String* source) {
  printf("Emitting LexJob for %zu bytes of ", source->length); print_string(filename); printf("\n");

  // @Lazy We should use a pool allocator.
  LexJob* job = malloc(sizeof(LexJob));
  job->base.type = JOB_LEX;
  job->filename = filename;
  job->source = source;

  pipeline_emit(job);
}

void pipeline_emit_parse_job(String* filename, TokenList* tokens) {
  printf("Emitting FileParseJob for %zu tokens of ", tokens->length); print_string(filename); printf("\n");

  // @Lazy We should use a pool allocator.
  ParseJob* job = malloc(sizeof(ParseJob));
  job->base.type = JOB_PARSE;
  job->filename = filename;
  job->tokens = tokens;

  pipeline_emit(job);
}

void pipeline_emit_typecheck_job(Declaration* decl) {
  printf("Emitting TypecheckJob for "); print_string(decl->name->source); printf(" in "); print_string(decl->name->file); printf(" line %zu\n", decl->name->line);

  // @Lazy We should use a pool allocator.
  TypecheckJob* job = malloc(sizeof(TypecheckJob));
  job->base.type = JOB_TYPECHECK;
  job->declaration = decl;

  pipeline_emit(job);
}

void pipeline_emit_optimize_job(Declaration* decl) {
  printf("Emitting OptimizeJob for "); print_string(decl->name->source); printf(" in "); print_string(decl->name->file); printf(" line %zu\n", decl->name->line);

  // @Lazy We should use a pool allocator.
  OptimizeJob* job = malloc(sizeof(OptimizeJob));
  job->base.type = JOB_OPTIMIZE;
  job->declaration = decl;

  pipeline_emit(job);
}

void pipeline_emit_bytecode_job(List* declarations) {
  printf("Emitting BytecodeJob for %zu declarations\n", declarations->size);

  // @Lazy We should use a pool allocator.
  BytecodeJob* job = malloc(sizeof(BytecodeJob));
  job->base.type = JOB_BYTECODE;
  // job->declaration = decl;

  pipeline_emit(job);
}

DEFINE(Job, SENTINEL, { JOB_SENTINEL });
void initialize_pipeline() {
  initialize_queue(&JOB_QUEUE, 1, 1);

  // Since we constantly re-enqueue incomplete work, this sentinel value will
  // allow us to detect unsolvable cycles and abort cleanly.
  pipeline_emit(SENTINEL);
}

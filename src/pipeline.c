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

void pipeline_emit_file_parse_job(String* filename, TokenList* tokens) {
  printf("Emitting FileParseJob for %zu tokens of ", tokens->length); print_string(filename); printf("\n");

  // @Lazy We should use a pool allocator.
  FileParseJob* job = malloc(sizeof(FileParseJob));
  job->base.type = JOB_PARSE_FILE;
  job->filename = filename;
  job->tokens = tokens;

  pipeline_emit(job);
}

DEFINE(Job, SENTINEL, { JOB_SENTINEL });
void initialize_pipeline() {
  initialize_queue(&JOB_QUEUE, 1, 1);

  // Since we constantly re-enqueue incomplete work, this sentinel value will
  // allow us to detect unsolvable cycles and abort cleanly.
  pipeline_emit(SENTINEL);
}

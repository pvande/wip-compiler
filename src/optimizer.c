bool perform_optimize_job(Job* job) {
  // @TODO Optimizations.

  pipeline_emit_bytecode_job(job->ws, job->file, job->node);

  return 1;
}

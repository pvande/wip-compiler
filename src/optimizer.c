bool perform_optimize_job(OptimizeJob* job) {
  // @TODO Optimizations.

  pipeline_emit_bytecode_job(job->node);

  return 1;
}

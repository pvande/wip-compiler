bool perform_read_job(ReadJob* job) {
  // @Lazy `filename.data` may not be naturally zero-terminated.
  char* file = to_zero_terminated_string(job->filename);
  String* source = file_read_all(file);
  free(file);

  if (source == NULL) {
    // @TODO: Record an error about not being able to find this file.
    free(job);
    return 0;
  }

  pipeline_emit_lex_job(job->filename, source);
  return 1;
}

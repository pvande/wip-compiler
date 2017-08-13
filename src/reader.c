bool perform_read_job(Job* job) {
  FileInfo* file = job->file;

  // @Lazy `filename.data` may not be naturally zero-terminated.
  char* filename = to_zero_terminated_string(file->filename);
  file->source = file_read_all(filename);
  free(filename);

  if (file->source == NULL) {
    // @TODO: Record an error about not being able to find this file.
    return 0;
  }

  pipeline_emit_lex_job(job->ws, file);
  return 1;
}

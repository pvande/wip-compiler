size_t file_read_all_into(const char* filename, String* str) {
  FILE* f = fopen(filename, "rb");
  if (!f) {
    return -1;
  }

  struct stat s;
  fstat(fileno(f), &s);

  str->length = s.st_size;
  str->data = malloc(s.st_size * sizeof(char));

  fread(str->data, s.st_size, 1, f);
  fclose(f);

  return s.st_size;
}

String* read_whole_file(const char* filename) {
  FILE* f = fopen(filename, "rb");
  if (!f) {
    return NULL;
  }

  struct stat s;
  fstat(fileno(f), &s);

  String* str = (String*) malloc(sizeof(String));
  str->length = s.st_size;
  str->data = malloc(s.st_size * sizeof(char));

  fread(str->data, s.st_size, 1, f);
  fclose(f);

  return str;
}

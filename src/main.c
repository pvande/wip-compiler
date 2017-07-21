#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>

// ** String ** //

typedef struct {
  uint64_t length;
  char*    data;
} String;

char* to_zero_terminated_string(String* str) {
  char* buffer = malloc(str->length * sizeof(char));

  memcpy(buffer, str->data, str->length);
  buffer[str->length] = '\0';

  return buffer;
}


// ** File ** //

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


// ** main ** //

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>...\n", argv[0]);
    return 1;
  }

  int num_files_to_process = argc - 1;
  char** files_to_process = argv + 1;

  for (int i = 0; i < num_files_to_process; i++) {
    String* result = read_whole_file(files_to_process[i]);

    if (result != NULL) {
      printf("%s", to_zero_terminated_string(result));
    }
  }
  return 0;
}

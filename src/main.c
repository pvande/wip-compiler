#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "src/string.c"
#include "src/file.c"
#include "src/table.c"

#include "src/tokenizer.c"

#ifndef TESTING

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>...\n", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i++) {
    String filename = { strlen(argv[i]), argv[i] };
    String* result = read_whole_file(argv[i]);

    if (result != NULL) {
      tokenize_string(&filename, result);
    }
  }

  return 0;
}

#endif

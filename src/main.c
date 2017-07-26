#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "src/string.c"
#include "src/file.c"
#include "src/table.c"
#include "src/list.c"

#include "src/tokenizer.c"
#include "src/parser.c"

#ifndef TESTING

int main(int argc, char** argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <filename>...\n", argv[0]);
    return 1;
  }

  ParserScope* scope = new_parser_scope();

  for (int i = 1; i < argc; i++) {
    String* file = new_string(argv[i]);
    String* result = read_whole_file(argv[i]);

    if (result != NULL) {
      TokenList* list = tokenize_string(file, result);

      parse_tokens(list, scope);
    }
  }

  return 0;
}

#endif

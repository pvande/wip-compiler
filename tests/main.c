#define TESTING 1

#include "src/main.c"

#define PRINT(V) _Generic((V), size_t: __print_size_t, int: __print_int, TokenType: __print_TokenType)(V)
#define GEN_PRINT(TYPE, SPECIFIER_STR) int __print_##TYPE(TYPE x) { return printf(SPECIFIER_STR, x);}
GEN_PRINT(size_t, "%ju");
GEN_PRINT(int, "%d");
GEN_PRINT(TokenType, "%d");

#define TEST(NAME) printf("\n\e[1;38m%s\n", NAME)
#define ASSERT_EQ(ACTUAL, EXPECTED, MSG)  if (ACTUAL == EXPECTED) { printf("  [\e[0;32mok\e[0m] %s\n", MSG); } else { printf("  [\e[0;31mnot ok\e[0m] %s\n    Expected \e[0;34m%s\e[0m == \e[0;32m", MSG, # ACTUAL); PRINT(EXPECTED); printf("\e[0m but got \e[0;31m"); PRINT(ACTUAL); printf("\e[0m\n\n"); dump_token_list(list); return; }

const char* ESCAPED[256] = {NULL};
void initialize_escape_sequences() {
  for (int i; i < 32; i++) {
    ESCAPED[i] = malloc(4 * sizeof(char));
    sprintf((char*) ESCAPED[i], "\\x%02X", i);
  }
  for (int i = 127; i < 256; i++) {
    ESCAPED[i] = malloc(4 * sizeof(char));
    sprintf((char*) ESCAPED[i], "\\x%02X", i);
  }

  ESCAPED['\0'] = "\\0";
  ESCAPED['\a'] = "\\a";
  ESCAPED['\b'] = "\\b";
  ESCAPED['\e'] = "\\e";
  ESCAPED['\f'] = "\\f";
  ESCAPED['\n'] = "\\n";
  ESCAPED['\r'] = "\\r";
  ESCAPED['\t'] = "\\t";
  ESCAPED['\v'] = "\\v";
  ESCAPED['\\'] = "\\\\";
  ESCAPED['"'] = "\\\"";
}

void print_string(String* str) {
  printf("\"");
  for (int i = 0; i < str->length; i++) {
    unsigned char c = str->data[i];

    if (ESCAPED[c] == NULL)
      printf("%c", c);
    else
      printf("%s", ESCAPED[c]);
  }
  printf("\"");
}

String* newString(char* s) {
  String* str = malloc(sizeof(String));

  str->length = strlen(s);
  str->data = s;

  return str;
}

#include "tests/tokenizer.c"

int main() {
  initialize_escape_sequences();

  printf("\nTOKENIZER TESTS\n");
  run_all_tokenizer_tests();

  printf("\nALL TESTS RUN\n");
  return 0;
}

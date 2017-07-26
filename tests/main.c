#define TESTING 1

#include "src/main.c"

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

void print_escaped_character(unsigned char c) {
  if (ESCAPED[c] == NULL)
    printf("%c", c);
  else
    printf("\e[1;30m%s\e[m", ESCAPED[c]);
}

void print_string(String* str) {
  printf("\"");
  for (int i = 0; i < str->length; i++) {
    unsigned char c = str->data[i];
    print_escaped_character(c);
  }
  printf("\"");
}

#define RAW(X, SIZE)  do { for (int i = 0; i < SIZE; i++) { print_escaped_character(((char*) X)[i]); } printf("\n"); } while (0)

#define PRINT(V) _Generic((V), void*: __print_ptr, String*: __print_ptr, size_t: __print_size_t, int: __print_int, TokenType: __print_TokenType)(V)
#define GEN_PRINT(TYPE, SPECIFIER_STR) int __print_##TYPE(TYPE x) { return printf(SPECIFIER_STR, x);}
GEN_PRINT(size_t, "%ju");
GEN_PRINT(int, "%d");
GEN_PRINT(TokenType, "%d");
void __print_ptr(void* x) { printf("0x%0X", (unsigned int)x); }

int __tests_run = 0;
int __assertions = 0;
int __failed_assertions = 0;

#define TEST(NAME) do { __tests_run += 1; printf("\n\e[1;38m%s\n", NAME); } while (0)
#define __ASSERT__(COND, MSG, FAILURE)  do { __assertions += 1; if (COND) { printf("  [\e[0;32mok\e[0m] %s\n", MSG); } else { __failed_assertions += 1; printf("  [\e[0;31mnot ok\e[0m] %s\n    ", MSG); FAILURE; return; } } while (0)
#define ASSERT_EQ(ACTUAL, EXPECTED, MSG)  __ASSERT__(ACTUAL == EXPECTED, MSG, { printf("Expected \e[0;34m%s\e[0m == \e[0;32m", # ACTUAL); PRINT(EXPECTED); printf("\e[0m but got \e[0;31m"); PRINT(ACTUAL); printf("\e[0m\n\n"); })
#define ASSERT_STR_EQ(ACTUAL, EXPECTED, MSG)  __ASSERT__(string_equals(ACTUAL, EXPECTED), MSG, { printf("Expected \e[0;32m"); print_string(EXPECTED); printf("\e[0m == \e[0;31m"); print_string(ACTUAL); printf("\e[0m\n\n"); })

#include "tests/table.c"
#include "tests/tokenizer.c"

int main() {
  initialize_escape_sequences();

  printf("\nTABLE TESTS\n");
  run_all_table_tests();

  printf("\nTOKENIZER TESTS\n");
  run_all_tokenizer_tests();

  printf("\n\e[0;32m%d\e[0m tests, \e[0;32m%d\e[0m assertions, \e[0;31m%d\e[0m failures\n", __tests_run, __assertions, __failed_assertions);
  return 0;
}

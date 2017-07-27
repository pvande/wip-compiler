#define TESTING 1

#include "src/main.c"

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
  // printf("\nTABLE TESTS\n");
  // run_all_table_tests();

  printf("\nTOKENIZER TESTS\n");
  run_all_tokenizer_tests();

  printf("\n\e[0;32m%d\e[0m tests, \e[0;32m%d\e[0m assertions, \e[0;31m%d\e[0m failures\n", __tests_run, __assertions, __failed_assertions);
  return 0;
}

#define TESTING 1

#include "src/main.c"

#define PRINT(V) _Generic((V), size_t: __print_size_t, int: __print_int, TokenType: __print_TokenType)(V)
#define GEN_PRINT(TYPE, SPECIFIER_STR) int __print_##TYPE(TYPE x) { return printf(SPECIFIER_STR, x);}
GEN_PRINT(size_t, "%ju");
GEN_PRINT(int, "%d");
GEN_PRINT(TokenType, "%d");

#define TEST(NAME) printf("TEST: %s\n", NAME)
#define ASSERT_EQ(ACTUAL, EXPECTED, MSG)  if (ACTUAL == EXPECTED) { printf("  \e[0;32mok\e[0m - %s\n", MSG); } else { printf("  \e[0;31mnot ok\e[0m - %s\n    Expected \e[0;34m%s\e[0m == \e[0;32m", MSG, # ACTUAL); PRINT(EXPECTED); printf("\e[0m but got \e[0;31m"); PRINT(ACTUAL); printf("\e[0m\n\n"); dump_token_list(list); return; }

const char* ESCAPED[256] = {NULL};
void initialize_escape_sequences() {
  for (int i; i < 32; i++) {
    ESCAPED[i] = malloc(4 * sizeof(char));
    sprintf((char*) ESCAPED[i], "\\x%02x", i);
  }
  for (int i = 127; i < 256; i++) {
    ESCAPED[i] = malloc(4 * sizeof(char));
    sprintf((char*) ESCAPED[i], "\\x%02x", i);
  }

  ESCAPED['\0'] = "\\0";
  ESCAPED['\a'] = "\\a";
  ESCAPED['\b'] = "\\b";
  ESCAPED['\e'] = "\\e";
  ESCAPED['\f'] = "\\f";
  ESCAPED['\r'] = "\\r";
  ESCAPED['\t'] = "\\t";
  ESCAPED['\v'] = "\\v";
  ESCAPED['\\'] = "\\\\";
  ESCAPED['"'] = "\\\"";
}

void print_string(String* str) {
  printf("\"");
  for (int i = 0; i < str->length; i++) {
    char c = str->data[i];

    if (ESCAPED[c] == NULL)
      printf("%c", c);
    else
      printf("%s", ESCAPED[c]);
  }
  printf("\"");
}

void dump_token(Token t) {
  printf("«Token type=%d file=", t.type);
  print_string(t.filename);
  printf(" line=%ju pos=%ju source=", t.line, t.pos);
  print_string(t.source);
  printf("»\n");
}

void dump_token_list(TokenList* list){
  if (list == NULL) {
    fprintf(stderr, "NULL TokenList returned!");
    return;
  }

  TokenList t = *list;
  printf("List [ %ju ]\n", t.length);

  for (uintmax_t i = 0; i < t.length; i++) {
    dump_token(t.tokens[i]);
  }
}


String* newString(char* s) {
  String* str = malloc(sizeof(String));

  str->length = strlen(s);
  str->data = s;

  return str;
}


void* tokenize(char* str) {
  return tokenize_string(newString("FILE"), newString(str));
}

void tokenize_test(char* str) {
  printf("\n--- BEGIN\n%s\n--- END\n", str);
  dump_token_list(tokenize(str));
}

void run_empty_tests() {
  TokenList* list;

  TEST("The empty string");
  list = tokenize("");
  ASSERT_EQ(list->length, 0, "it should create no tokens");
}

void run_whitespace_tests() {
  TokenList* list;

  TEST("A single space");
  list = tokenize(" ");
  ASSERT_EQ(list->length, 1, "creates a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "creates a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "creates a token with a length of 1");

  TEST("Multiple contiguous spaces");
  list = tokenize("   ");
  ASSERT_EQ(list->length, 1, "create a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "create a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "create a token with a length of 3");

  TEST("Mixed tabs and spaces (space first)");
  list = tokenize(" \t\t  ");
  ASSERT_EQ(list->length, 1, "create a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "create a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "create a token with a length of 5");

  TEST("Mixed tabs and spaces (tab first)");
  list = tokenize("\t  \t\t");
  ASSERT_EQ(list->length, 1, "create multiple tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "create a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "create a token with a length of 5");
}

void run_newline_tests() {
  TokenList* list;

  TEST("A single newline");
  list = tokenize("\n");
  ASSERT_EQ(list->length, 1, "creates a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NEWLINE, "creates a newline token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "creates a token with a length of 1");

  TEST("Multiple contiguous newlines");
  list = tokenize("\n\n\n");
  ASSERT_EQ(list->length, 1, "create a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NEWLINE, "create a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "create a token with a length of 3");

  TEST("Mixed newlines and whitespace");
  list = tokenize(" \n\n  ");
  ASSERT_EQ(list->length, 3, "creates multiple token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "create a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "with a length of 1");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 2, "with a length of 2");
  ASSERT_EQ(list->tokens[2].type, TOKEN_TYPE_WHITESPACE, "followed by a whitespace token");
  ASSERT_EQ(list->tokens[2].source->length, 2, "with a length of 2");
}

void run_comment_tests() {
  TokenList* list;

  TEST("A single line comment");
  list = tokenize("// This is a comment.\n");
  ASSERT_EQ(list->length, 2, "creates two tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_COMMENT, "a comment token");
  ASSERT_EQ(list->tokens[0].source->length, 21, "with a length of 21");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "with a length of 1");

  TEST("A comment not followed by a newline");
  list = tokenize("// This is a comment.");
  ASSERT_EQ(list->length, 1, "creates one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_COMMENT, "a comment token");
  ASSERT_EQ(list->tokens[0].source->length, 21, "with a length of 21");
}

int main() {
  initialize_escape_sequences();

  run_empty_tests();
  run_whitespace_tests();
  run_newline_tests();
  run_comment_tests();

  // // Number tests.
  // tokenize_test("1234");
  // tokenize_test("12 34");
  // tokenize_test("01234");
  // tokenize_test("0x12b34");
  // tokenize_test("0b1011");
  // tokenize_test("0b10_01");
  // tokenize_test("0b10_201"); // Two tokens, `0b10_` and `201`.
  //
  // // String tests.
  // tokenize_test("\"\"");
  // tokenize_test("\"1\"");
  // tokenize_test("\"abc\"");
  // tokenize_test("\"\\\\\"");
  // tokenize_test(" \"\\\"\" ");
  // tokenize_test("\"\n\"");
  // tokenize_test("\"\n\\n\"");

  printf("\n--- DONE\n");
  return 0;
}

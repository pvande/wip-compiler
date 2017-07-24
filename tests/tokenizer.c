// ** Utilities ** //

void* tokenize(char* str) {
  return tokenize_string(newString("FILE"), newString(str));
}

void tokenize_test(char* str) {
  printf("\n--- BEGIN\n%s\n--- END\n", str);
  dump_token_list(tokenize(str));
}

// ** Tests ** //

void tokenizer_empty_tests() {
  TokenList* list;

  TEST("The empty string");
  list = tokenize("");
  ASSERT_EQ(list->length, 0, "contains no tokens");
}

void tokenizer_whitespace_tests() {
  TokenList* list;

  TEST("A single space");
  list = tokenize(" ");
  ASSERT_EQ(list->length, 1, "has a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "of length 1");

  TEST("Multiple contiguous spaces");
  list = tokenize("   ");
  ASSERT_EQ(list->length, 1, "has a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");

  TEST("Mixed tabs and spaces (space first)");
  list = tokenize(" \t\t  ");
  ASSERT_EQ(list->length, 1, "has a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");

  TEST("Mixed tabs and spaces (tab first)");
  list = tokenize("\t  \t\t");
  ASSERT_EQ(list->length, 1, "has multiple tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");
}

void tokenizer_newline_tests() {
  TokenList* list;

  TEST("A single newline");
  list = tokenize("\n");
  ASSERT_EQ(list->length, 1, "has a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NEWLINE, "a newline token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "of length 1");

  TEST("Multiple contiguous newlines");
  list = tokenize("\n\n\n");
  ASSERT_EQ(list->length, 1, "has a single token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NEWLINE, "a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");

  TEST("Mixed newlines and whitespace");
  list = tokenize(" \n\n  ");
  ASSERT_EQ(list->length, 3, "has multiple token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_WHITESPACE, "a whitespace token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "of length of 1");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 2, "of length of 2");
  ASSERT_EQ(list->tokens[2].type, TOKEN_TYPE_WHITESPACE, "followed by a whitespace token");
  ASSERT_EQ(list->tokens[2].source->length, 2, "of length of 2");
}

void tokenizer_comment_tests() {
  TokenList* list;

  TEST("A single line comment");
  list = tokenize("// This is a comment.\n");
  ASSERT_EQ(list->length, 2, "has two tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_COMMENT, "a comment token");
  ASSERT_EQ(list->tokens[0].source->length, 21, "of length 21");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A comment not followed by a newline");
  list = tokenize("// This is a comment.");
  ASSERT_EQ(list->length, 1, "has one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_COMMENT, "a comment token");
  ASSERT_EQ(list->tokens[0].source->length, 21, "of length 21");
}


void run_all_tokenizer_tests() {
  tokenizer_empty_tests();
  tokenizer_whitespace_tests();
  tokenizer_newline_tests();
  tokenizer_comment_tests();

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
}

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

void tokenizer_numeric_tests() {
  TokenList* list;

  TEST("A decimal number");
  list = tokenize("123");
  ASSERT_EQ(list->length, 1, "has one tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");

  TEST("A decimal number with underscores");
  list = tokenize("123_456_789");
  ASSERT_EQ(list->length, 1, "has one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 11, "of length 11");

  TEST("A decimal number with leading zeroes");
  list = tokenize("0000012345");
  ASSERT_EQ(list->length, 1, "has one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 10, "of length 10");


  TEST("A hexidecimal number");
  list = tokenize("0xDEADBEEF");
  ASSERT_EQ(list->length, 1, "has one tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_HEX, "a hex number token");
  ASSERT_EQ(list->tokens[0].source->length, 10, "of length 10");

  TEST("A hexidecimal number with underscores");
  list = tokenize("0x123_FF_789");
  ASSERT_EQ(list->length, 1, "has one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_HEX, "a hex number token");
  ASSERT_EQ(list->tokens[0].source->length, 12, "of length 12");

  TEST("A hexidecimal number with lowercased digits");
  list = tokenize("0xabc_de_f89");
  ASSERT_EQ(list->length, 1, "has one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_HEX, "a hex number token");
  ASSERT_EQ(list->tokens[0].source->length, 12, "of length 12");


  TEST("A binary number");
  list = tokenize("0b010");
  ASSERT_EQ(list->length, 1, "has one tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_BINARY, "a binary number token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");

  TEST("A binary number with underscores");
  list = tokenize("0b0010_0011");
  ASSERT_EQ(list->length, 1, "has one token");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_BINARY, "a binary number token");
  ASSERT_EQ(list->tokens[0].source->length, 11, "of length 11");


  TEST("A 'decimal' number containing hex digits");
  list = tokenize("00000FF12345");
  ASSERT_EQ(list->length, 2, "has two tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 7, "of length 7");

  TEST("A 'decimal' number containing letters");
  list = tokenize("00000ZZ12345");
  ASSERT_EQ(list->length, 2, "has two tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 7, "of length 7");

  TEST("A 'hexidecimal' number with leading zeroes");
  list = tokenize("000x0000012345");
  ASSERT_EQ(list->length, 2, "has two tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 11, "of length 11");

  TEST("A 'binary' number with leading zeroes");
  list = tokenize("000x000001");
  ASSERT_EQ(list->length, 2, "has two tokens");
  ASSERT_EQ(list->tokens[0].type, TOKEN_TYPE_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_TYPE_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 7, "of length 7");
}


void run_all_tokenizer_tests() {
  tokenizer_empty_tests();
  tokenizer_whitespace_tests();
  tokenizer_newline_tests();
  tokenizer_comment_tests();
  tokenizer_numeric_tests();

  // // String tests.
  // tokenize_test("\"\"");
  // tokenize_test("\"1\"");
  // tokenize_test("\"abc\"");
  // tokenize_test("\"\\\\\"");
  // tokenize_test(" \"\\\"\" ");
  // tokenize_test("\"\n\"");
  // tokenize_test("\"\n\\n\"");
}

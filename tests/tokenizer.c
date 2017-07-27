void* tokenize(char* str) {
  return tokenize_string(new_string("FILE"), new_string(str));
}

void tokenizer_empty_tests() {
  TokenList* list;

  TEST("The empty string");
  list = tokenize("");
  ASSERT_EQ(list->length, 0, "has correct list length");
}

void tokenizer_whitespace_tests() {
  TokenList* list;

  TEST("A single space");
  list = tokenize(" ");
  ASSERT_EQ(list->length, 0, "has correct list length");

  TEST("Multiple contiguous spaces");
  list = tokenize("   ");
  ASSERT_EQ(list->length, 0, "has correct list length");

  TEST("Mixed tabs and spaces (space first)");
  list = tokenize(" \t\t  ");
  ASSERT_EQ(list->length, 0, "has correct list length");

  TEST("Mixed tabs and spaces (tab first)");
  list = tokenize("\t  \t\t");
  ASSERT_EQ(list->length, 0, "has correct list length");
}

void tokenizer_newline_tests() {
  TokenList* list;

  TEST("A single newline");
  list = tokenize("\n");
  ASSERT_EQ(list->length, 1, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NEWLINE, "a newline token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "of length 1");

  TEST("Multiple contiguous newlines");
  list = tokenize("\n\n\n");
  ASSERT_EQ(list->length, 1, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NEWLINE, "a newline token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");

  TEST("Mixed newlines and whitespace");
  list = tokenize(" \n\n  \n  \n  ");
  ASSERT_EQ(list->length, 1, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[0].source->length, 10, "of length of 2");
}

void tokenizer_comment_tests() {
  TokenList* list;

  TEST("A single line comment");
  list = tokenize("// This is a comment.\n");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_COMMENT, "a comment token");
  ASSERT_EQ(list->tokens[0].source->length, 21, "of length 21");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A comment not followed by a newline");
  list = tokenize("// This is a comment.");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_COMMENT, "a comment token");
  ASSERT_EQ(list->tokens[0].source->length, 21, "of length 21");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");
}

void tokenizer_numeric_tests() {
  TokenList* list;

  TEST("A decimal number");
  list = tokenize("123");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A decimal number with underscores");
  list = tokenize("123_456_789");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 11, "of length 11");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A decimal number with leading zeroes");
  list = tokenize("0000012345");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 10, "of length 10");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");


  TEST("A hexidecimal number");
  list = tokenize("0xDEADBEEF");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_HEX, "a hex number token");
  ASSERT_EQ(list->tokens[0].source->length, 10, "of length 10");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A hexidecimal number with underscores");
  list = tokenize("0x123_FF_789");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_HEX, "a hex number token");
  ASSERT_EQ(list->tokens[0].source->length, 12, "of length 12");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A hexidecimal number with lowercased digits");
  list = tokenize("0xabc_de_f89");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_HEX, "a hex number token");
  ASSERT_EQ(list->tokens[0].source->length, 12, "of length 12");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");


  TEST("A binary number");
  list = tokenize("0b010");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_BINARY, "a binary number token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A binary number with underscores");
  list = tokenize("0b0010_0011");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_BINARY, "a binary number token");
  ASSERT_EQ(list->tokens[0].source->length, 11, "of length 11");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");


  TEST("A 'decimal' number containing hex digits");
  list = tokenize("00000FF12345");
  ASSERT_EQ(list->length, 3, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");
  ASSERT_EQ(list->tokens[1].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 7, "of length 7");
  ASSERT_EQ(list->tokens[2].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[2].source->length, 1, "of length 1");

  TEST("A 'decimal' number containing letters");
  list = tokenize("00000ZZ12345");
  ASSERT_EQ(list->length, 3, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 5, "of length 5");
  ASSERT_EQ(list->tokens[1].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 7, "of length 7");
  ASSERT_EQ(list->tokens[2].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[2].source->length, 1, "of length 1");

  TEST("A 'hexidecimal' number with leading zeroes");
  list = tokenize("000x0000012345");
  ASSERT_EQ(list->length, 3, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 11, "of length 11");
  ASSERT_EQ(list->tokens[2].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[2].source->length, 1, "of length 1");

  TEST("A 'binary' number with leading zeroes");
  list = tokenize("000x000001");
  ASSERT_EQ(list->length, 3, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[1].source->length, 7, "of length 7");
  ASSERT_EQ(list->tokens[2].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[2].source->length, 1, "of length 1");
}

void tokenizer_string_tests() {
  TokenList* list;

  TEST("A simple string");
  list = tokenize("\"this is a string\"");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_STRING, "a string token");
  ASSERT_EQ(list->tokens[0].source->length, 18, "of length 18");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A string containing escaped characters");
  list = tokenize("\"\\a\\b\\f\\n\\\"\\'\\x20\"");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_STRING, "a string token");
  ASSERT_EQ(list->tokens[0].source->length, 18, "of length 18");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A string without closing quotes before a newline");
  list = tokenize("\"Newline ahead\n");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_STRING, "a string token");
  ASSERT_EQ(list->tokens[0].source->length, 14, "of length 14");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A string ending with a backslash without closing quotes before a newline");
  list = tokenize("\"Newline ahead\\\\\n");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_STRING, "a string token");
  ASSERT_EQ(list->tokens[0].source->length, 16, "of length 16");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("An unterminated string");
  list = tokenize("\"EOF ahead");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_STRING, "a string token");
  ASSERT_EQ(list->tokens[0].source->length, 10, "of length 10");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");
}

void tokenizer_operator_tests() {
  TokenList* list;

  TEST("A simple operator example");
  list = tokenize("1 + 2");
  ASSERT_EQ(list->length, 4, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[0].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[1].type, TOKEN_OPERATOR, "an operator token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[2].type, TOKEN_NUMBER_DECIMAL, "a decimal number token");
  ASSERT_EQ(list->tokens[2].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[3].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[3].source->length, 1, "of length 1");

  TEST("A more complex operator example");
  list = tokenize("foo(1 + 2, 3,*bar)");
  ASSERT_EQ(list->length, 12, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_OPERATOR, "followed by an operator token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[2].type, TOKEN_NUMBER_DECIMAL, "followed by a decimal number token");
  ASSERT_EQ(list->tokens[2].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[3].type, TOKEN_OPERATOR, "followed by an operator token");
  ASSERT_EQ(list->tokens[3].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[4].type, TOKEN_NUMBER_DECIMAL, "followed by a decimal number token");
  ASSERT_EQ(list->tokens[4].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[5].type, TOKEN_OPERATOR, "followed by an operator token");
  ASSERT_EQ(list->tokens[5].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[6].type, TOKEN_NUMBER_DECIMAL, "followed by a decimal number token");
  ASSERT_EQ(list->tokens[6].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[7].type, TOKEN_OPERATOR, "followed by an operator token");
  ASSERT_EQ(list->tokens[7].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[8].type, TOKEN_OPERATOR, "followed by an operator token");
  ASSERT_EQ(list->tokens[8].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[9].type, TOKEN_IDENTIFIER, "followed by an identifier token");
  ASSERT_EQ(list->tokens[9].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[10].type, TOKEN_OPERATOR, "followed by an operator token");
  ASSERT_EQ(list->tokens[10].source->length, 1, "of length 1");
  ASSERT_EQ(list->tokens[11].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[11].source->length, 1, "of length 1");
}

void tokenizer_identifier_tests() {
  TokenList* list;

  TEST("A simple identifier");
  list = tokenize("foo");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[0].source->length, 3, "of length 3");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");

  TEST("A unicode identifier");
  list = tokenize("👉🏽🚷");
  ASSERT_EQ(list->length, 2, "has correct list length");
  ASSERT_EQ(list->tokens[0].type, TOKEN_IDENTIFIER, "an identifier token");
  ASSERT_EQ(list->tokens[0].source->length, 12, "of length 12");
  ASSERT_EQ(list->tokens[1].type, TOKEN_NEWLINE, "followed by a newline token");
  ASSERT_EQ(list->tokens[1].source->length, 1, "of length 1");
}

void run_all_tokenizer_tests() {
  tokenizer_empty_tests();
  tokenizer_whitespace_tests();
  tokenizer_newline_tests();
  tokenizer_comment_tests();
  tokenizer_numeric_tests();
  tokenizer_string_tests();
  tokenizer_operator_tests();
  tokenizer_identifier_tests();
}

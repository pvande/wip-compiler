#define TESTING 1

#include "src/main.c"

void print_string(String* str) {
  printf("\"%.*s\" (%ju)", (int) str->length, str->data, str->length);
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


void tokenize_test(char* str) {
  printf("\n--- BEGIN\n%s\n--- END\n", str);
  dump_token_list(tokenize_string(newString("FILE"), newString(str)));
}

int main() {
  // Whitespace tests.
  tokenize_test("");
  tokenize_test(" ");
  tokenize_test("   ");
  tokenize_test("\t");
  tokenize_test("\t ");
  tokenize_test(" \t");
  tokenize_test(" \t  ");
  tokenize_test("\n");
  tokenize_test(" \n");
  tokenize_test(" \n\t \n\n ");

  // Number tests.
  tokenize_test("1234");
  tokenize_test("12 34");
  tokenize_test("01234");
  tokenize_test("0x1234");
  tokenize_test("0b1234");
  tokenize_test("0b12_34");
  printf("--- DONE\n");
  return 0;
}

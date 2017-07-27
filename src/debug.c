const char* ESCAPED[256] = {
  "\\x00", "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a",
  "\\b",   "\\t",   "\\n",   "\\v",   "\\f",   "\\r",   "\\x0E", "\\x0F",
  "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17",
  "\\x18", "\\x19", "\\x1A", "\\e",   "\\x1C", "\\x1D", "\\x1E", "\\x1F",
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    "\\x7F",
  "\\x80", "\\x81", "\\x82", "\\x83", "\\x84", "\\x85", "\\x86", "\\x87",
  "\\x88", "\\x89", "\\x8A", "\\x8B", "\\x8C", "\\x8D", "\\x8E", "\\x8F",
  "\\x90", "\\x91", "\\x92", "\\x93", "\\x94", "\\x95", "\\x96", "\\x97",
  "\\x98", "\\x99", "\\x9A", "\\x9B", "\\x9C", "\\x9D", "\\x9E", "\\x9F",
  "\\xA0", "\\xA1", "\\xA2", "\\xA3", "\\xA4", "\\xA5", "\\xA6", "\\xA7",
  "\\xA8", "\\xA9", "\\xAA", "\\xAB", "\\xAC", "\\xAD", "\\xAE", "\\xAF",
  "\\xB0", "\\xB1", "\\xB2", "\\xB3", "\\xB4", "\\xB5", "\\xB6", "\\xB7",
  "\\xB8", "\\xB9", "\\xBA", "\\xBB", "\\xBC", "\\xBD", "\\xBE", "\\xBF",
  "\\xC0", "\\xC1", "\\xC2", "\\xC3", "\\xC4", "\\xC5", "\\xC6", "\\xC7",
  "\\xC8", "\\xC9", "\\xCA", "\\xCB", "\\xCC", "\\xCD", "\\xCE", "\\xCF",
  "\\xD0", "\\xD1", "\\xD2", "\\xD3", "\\xD4", "\\xD5", "\\xD6", "\\xD7",
  "\\xD8", "\\xD9", "\\xDA", "\\xDB", "\\xDC", "\\xDD", "\\xDE", "\\xDF",
  "\\xE0", "\\xE1", "\\xE2", "\\xE3", "\\xE4", "\\xE5", "\\xE6", "\\xE7",
  "\\xE8", "\\xE9", "\\xEA", "\\xEB", "\\xEC", "\\xED", "\\xEE", "\\xEF",
  "\\xF0", "\\xF1", "\\xF2", "\\xF3", "\\xF4", "\\xF5", "\\xF6", "\\xF7",
  "\\xF8", "\\xF9", "\\xFA", "\\xFB", "\\xFC", "\\xFD", "\\xFE", "\\xFF",
};

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
#define PRINT(V) _Generic((V), \
  void*: print_pointer, \
  String*: print_string, \
  size_t: __print_size_t, \
  TokenType: __print_int, \
  int: __print_int \
)(V)
#define GEN_PRINT(TYPE, SPECIFIER_STR) int __print_##TYPE(TYPE x) { return printf(SPECIFIER_STR, x);}
GEN_PRINT(size_t, "%ju");
GEN_PRINT(int, "%d");

void print_pointer(void* x) {
  printf("0x%0X", (unsigned int) x);
}

void print_token(Token t) {
  printf("«Token type=%d file=", t.type);
  print_string(t.file);
  printf(" line=%ju pos=%ju source=", t.line, t.pos);
  print_string(t.source);
  printf("»\n");
}

void print_token_list(TokenList* list){
  if (list == NULL) {
    fprintf(stderr, "NULL TokenList returned!");
    return;
  }

  TokenList t = *list;
  printf("List [ %ju ]\n", t.length);

  for (uintmax_t i = 0; i < t.length; i++) {
    print_token(t.tokens[i]);
  }
}

void debug_table(Table* t) {
  printf("{ [%zu of %zu]\n", t->size, t->capacity);
  for (int i = 0; i < t->capacity; i++) {
    if (t->occupied[i]) {
      printf("  ");
      print_string(t->keys[i]);
      printf(" => 0x%0X\n", (unsigned int) t->values[i]);
    } else {
      printf("  ---\n");
    }
  }
  printf("}\n");
}


void print_table(Table* t) {
  printf("{");
  for (int i = 0; i < t->capacity; i++) {
    if (!t->occupied[i]) continue;

    print_string(t->keys[i]);
    printf(" => ");
    print_pointer(t->values[i]);
    printf(", ");
  }
  printf("}");
}

// void print_list(List* t) {
//   printf("[");
//   for (int i = 0; i < t->count; i++) {
//     printf("  ");
//     print_string(t->keys[i]);
//     printf(" => ")
//     print_pointer(t->values[i]);
//   }
//   printf("]");
// }

void print_scope_declarations_table_with_indentation(ParserScope* scope, int indent) {
  printf("{\n");
  for (int i = 0; i < scope->declarations->capacity; i++) {
    if (!scope->declarations->occupied[i]) continue;

    for (int i = 0; i < indent; i++) printf("  ");
    print_string(scope->declarations->keys[i]);
    printf(" => ");
    print_pointer(scope->declarations->values[i]);
    printf("\n");
  }

  for (int i = 0; i < indent - 1; i++) printf("  ");
  printf("}\n");
}

void print_scope(ParserScope* scope) {
  printf("Scope: ");
  int indentation = 1;
  do {
    print_scope_declarations_table_with_indentation(scope, indentation);
    indentation += 1;
    scope = scope->parent_scope;
  } while (scope != NULL);
}

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
#define RAW(X, SIZE)  do { for (int i = 0; i < SIZE; i++) { print_escaped_character(((char*) X)[i]); } printf("\n"); } while (0)

void print_string(String* str) {
  printf("\"");
  for (int i = 0; i < str->length; i++) {
    unsigned char c = str->data[i];
    print_escaped_character(c);
  }
  printf("\"");
}

void print_symbol(Symbol sym) {
  print_string(symbol_lookup(sym));
}

#define PRINT(V) _Generic((V), \
  void*: print_pointer, \
  List*: print_pointer, \
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

void inspect_token(Token t) {
  printf("«Token type=%d file=", t.type);
  print_string(&t.file);
  printf(" line=%ju pos=%ju source=", t.line, t.pos);
  print_string(&t.source);
  printf("»\n");
}

void print_tokenized_file(TokenizedFile* list){
  if (list == NULL) {
    fprintf(stderr, "NULL TokenizedFile returned!");
    return;
  }

  TokenizedFile t = *list;
  printf("List [ %ju ]\n", t.count);

  for (uintmax_t i = 0; i < t.count; i++) {
    inspect_token(t.tokens[i]);
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
  int printed = 0;
  printf("{");
  for (int i = 0; i < t->capacity; i++) {
    if (!t->occupied[i]) continue;
    if (printed) printf(", ");

    printed = 1;
    print_string(t->keys[i]);
    printf(" => ");
    print_pointer(t->values[i]);
  }
  printf("}");
}

void print_list(List* list) {
  printf("[");
  for (int i = 0; i < list->size; i++) {
    if (i > 0) printf(", ");
    print_pointer(list_get(list, i));
  }
  printf("]");
}

void print_declaration();

void print_token(Token* token) {
  printf("%s", to_zero_terminated_string(&token->source));
}

void print_ast_type(AstType* type) {
  print_symbol(type->name);
}

void print_expression(AstExpression* _expr) {
  if (_expr->type == EXPR_IDENT) {
    IdentifierExpression* expr = (void*) _expr;
    print_token(expr->identifier);
    return;
  } else if (_expr->type == EXPR_LITERAL) {
    LiteralExpression* expr = (void*) _expr;
    print_token(expr->literal);
    return;
  } else if (_expr->type == EXPR_FUNCTION) {
    FunctionExpression* expr = (void*) _expr;
    printf("(");
    for (int i = 0; i < expr->arguments->size; i++) {
      if (i > 0) printf(", ");
      print_declaration(list_get(expr->arguments, i));
    }
    printf(")");
    printf(" => ");

    print_ast_type(&expr->returns);

    printf(" {}");
  }

  printf("(");
  if (_expr->type == EXPR_UNARY_OP) {
    UnaryOpExpression* expr = (void*) _expr;
    if (expr->operator->source.data[0] != '(') print_token(expr->operator);
    print_expression(expr->rhs);
  } else if (_expr->type == EXPR_BINARY_OP) {
    BinaryOpExpression* expr = (void*) _expr;
    print_expression(expr->lhs);
    printf(" ");
    print_token(expr->operator);
    printf(" ");
    print_expression(expr->rhs);
  }
  printf(")");
}

void print_declaration(AstDeclaration* decl) {
  print_symbol(decl->name);
  printf(" : ");

  if (decl->type == NULL) {
    printf("___");
  } else {
    print_ast_type(decl->type);
  }

  printf(" = ");

  if (decl->value == NULL) {
    printf("NULL");
  } else {
    print_expression(decl->value);
  }
}

void print_declaration_list(List* list) {
  printf("[\n");
  for (int i = 0; i < list->size; i++) {
    AstDeclaration* decl = list_get(list, i);
    printf("    ");
    print_declaration(decl);
    printf("\n");
  }
  printf("  ]");
}

void print_scope(ParserScope* scope) {
  printf("\n\nScope: ");
  print_declaration_list(scope->declarations);
}

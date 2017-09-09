static char* ESCAPED[256] = {
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

void print_dotsafe_escaped_character(unsigned char c) {
  if (ESCAPED[c] == NULL)
    printf("%c", c);
  else
    printf("%s", ESCAPED[c]);
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

void print_dotsafe_string(String* str) {
  for (int i = 0; i < str->length; i++) {
    unsigned char c = str->data[i];
    if (c == '"') {
      printf("&quot;");
    } else if (c == ' ') {
      printf("&nbsp;");
    } else if (c == '<') {
      printf("&lt;");
    } else if (c == '>') {
      printf("&gt;");
    } else if (c == '{') {
      printf("&#123;");
    } else if (c == '}') {
      printf("&#125;");
    } else if (c == '\n') {
      printf("<BR ALIGN=\"LEFT\"/>");
    } else {
      print_dotsafe_escaped_character(c);
    }
  }
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
  int: __print_int, \
  char: __print_char, \
  long: __print_long \
)(V)
#define GEN_PRINT(TYPE, SPECIFIER_STR) int __print_##TYPE(TYPE x) { return printf(SPECIFIER_STR, x);}
GEN_PRINT(size_t, "%ju");
GEN_PRINT(long, "%zu");
GEN_PRINT(int, "%d");
GEN_PRINT(char, "%c");


char* _ast_node_type(AstNode* node) {
  switch (node->type) {
    case NODE_ASSIGNMENT:
      return "ASSIGNMENT";
    case NODE_CONDITIONAL:
      return "CONDITIONAL";
    case NODE_COMPOUND:
      return "COMPOUND";
    case NODE_DECLARATION:
      return "DECLARATION";
    case NODE_EXPRESSION:
      return "EXPRESSION";
    // case NODE_LOOP:
    //   return "LOOP";
    case NODE_RECOVERY:
      return "RECOVERY";
    case NODE_TYPE:
      return "TYPE";
    default:
      return "UNKNOWN";
  }
}

void print_pointer(void* x) {
  printf("0x%0X", (unsigned int) x);
}

void inspect_token(Token t) {
  printf("«Token type=%d line=%ju pos=%ju source=", t.type, t.line, t.pos);
  print_string(&t.source);
  printf("»\n");
}

void inspect_ast_node(AstNode* node) {
  printf("«AstNode 0x%X type=%s flags=%x id=%zu from=%zu,%zu to=%zu,%zu ident=%d type=%x bytecode_id=%zx»", (unsigned int) node, _ast_node_type(node), node->flags, node->id, node->from.line + 1, node->from.pos + 1, node->to.line + 1, node->to.pos + 1, (int) node->ident, (unsigned int) node->typeclass, node->bytecode_id);
}

void print_tokenized_file(TokenizedFile* list){
  if (list == NULL) {
    fprintf(stderr, "NULL TokenizedFile returned!");
    return;
  }

  TokenizedFile t = *list;
  printf("List [ %ju ]\n", t.length);

  for (uintmax_t i = 0; i < t.length; i++) {
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
  for (int i = 0; i < list->length; i++) {
    if (i > 0) printf(", ");
    print_pointer(list_get(list, i));
  }
  printf("]");
}

void print_token(Token* token) {
  printf("«Token(%d) line=%zu pos=%zu source=\"%s\" literal_type=%d is_well_formed=%d»",
         token->type,
         token->line,
         token->pos,
         to_zero_terminated_string(&token->source),
         token->literal_type,
         token->is_well_formed);
}

void print_ast_node_type(AstNode* node) {
  switch (node->type) {
    case NODE_ASSIGNMENT:
      printf("ASSIGNMENT");
      break;
    case NODE_CONDITIONAL:
      printf("CONDITIONAL");
      break;
    case NODE_COMPOUND:
      printf("COMPOUND(%zu)", node->body_length);
      break;
    case NODE_DECLARATION:
      printf("DECLARATION(");
      print_string(symbol_lookup(node->ident));
      printf(")");
      break;
    case NODE_EXPRESSION:
      printf("EXPRESSION");
      break;
    // case NODE_LOOP:
    //   printf("LOOP");
    //   break;
    case NODE_RECOVERY:
      printf("RECOVERY");
      break;
    case NODE_TYPE:
      printf("TYPE(");
      print_string(&node->source);
      printf(")");
      break;
    default:
      printf("UNKNOWN");
  }
}

void print_scope(Scope* scope) {
  printf("[");
  for (size_t i = 0; i < scope->declarations.length; i++) {
    AstNode* node = list_get(&scope->declarations, i);

    if (i > 0) printf(", ");
    print_string(symbol_lookup(node->ident));
  }

  if (scope->parent != NULL) {
    printf(" ");
    print_scope(scope->parent);
  }

  printf("]");
}

void print_ast_node_as_tree(String* lines, AstNode* node) {
  if (node == NULL) return;

  printf("[");
  print_ast_node_type(node);
  printf("]");
  printf("  ");
  if (node->scope != NULL) print_scope(node->scope);
  printf("\n");
  for (int line_number = node->from.line; line_number <= node->to.line; line_number++) {
    String* line = &lines[line_number];
    int mark_from = (node->from.line == line_number) ? node->from.pos : 0;
    int mark_to = (node->to.line == line_number) ? node->to.pos : line->length;

    for (int i = 0; i < mark_from; i++) {
      printf("%c", line->data[i]);
    }
    if (node->flags & NODE_CONTAINS_ERROR) {
      printf("\e[0;41m");
    } else {
      printf("\e[0;44m");
    }
    if (node->from.line == line_number) printf("«");
    for (int i = mark_from; i < mark_to; i++) {
      printf("%c", line->data[i]);
    }
    if (node->to.line == line_number) printf("»");
    printf("\e[0m");
    for (int i = mark_to; i < line->length; i++) {
      printf("%c", line->data[i]);
    }

    printf("\n");
  }
  printf("\n");

  if (node->flags & NODE_CONTAINS_LHS) {
    print_ast_node_as_tree(lines, node->lhs);
  }

  if (node->flags & NODE_CONTAINS_RHS) {
    print_ast_node_as_tree(lines, node->rhs);
  }

  for (int i = 0; i < node->body_length; i++) {
    print_ast_node_as_tree(lines, &node->body[i]);
  }
}

void print_ast_node_as_dot(String* lines, AstNode* node) {
  if (node == NULL) return;

  String source;
  source.data = lines[node->from.line].data + node->from.pos;
  source.length = 0;
  if (node->from.line != node->to.line) {
    source.length += lines[node->from.line].length - node->from.pos + 1;
    for (int line_number = node->from.line + 1; line_number < node->to.line; line_number++) {
      source.length += lines[line_number].length + 1;
    }
    source.length += node->to.pos;
  } else {
    source.length += node->to.pos - node->from.pos;
  }

  printf("node_%zu [shape=record, label=<<TABLE><TR><TD ALIGN=\"center\">%s</TD></TR><TR><TD ALIGN=\"left\">", node->id, _ast_node_type(node));
  print_dotsafe_string(&source);
  printf("<BR ALIGN=\"LEFT\"/>");
  printf("</TD></TR></TABLE>>]\n");

  if (node->flags == NODE_CONTAINS_LHS) {
    print_ast_node_as_dot(lines, node->lhs);
    printf("node_%zu -> node_%zu [label=lhs]\n", node->id, node->lhs->id);
  }

  if (node->flags == NODE_CONTAINS_RHS) {
    print_ast_node_as_dot(lines, node->rhs);
    if (node->rhs != NULL) {
      printf("node_%zu -> node_%zu [label=rhs]\n", node->id, node->rhs->id);
    }
  }

  if (node->body_length > 0) {
    printf("subgraph node_%zu_body {\n", node->id);
    printf("color=grey\n");
    for (int i = 0; i < node->body_length; i++) {
      print_ast_node_as_dot(lines, &node->body[i]);
    }
    printf("}\n");
    for (int i = 0; i < node->body_length; i++) {
      printf("node_%zu -> node_%zu [label=body]\n", node->id, node->body[i].id);
    }
  }
}

void print_ast_node_as_sexpr(String* lines, AstNode* node, int indent) {
  for (int i = 0; i < indent; i++) printf("  ");
  printf("(%s", _ast_node_type(node));
  if (node->flags & NODE_CONTAINS_IDENT) {
    printf(" ");
    print_symbol(node->ident);
  }
  if (node->flags & NODE_CONTAINS_SOURCE) {
    printf(" ");
    print_string(&node->source);
  }
  if (node->flags & NODE_CONTAINS_LHS) {
    printf("\n");
    print_ast_node_as_sexpr(lines, node->lhs, indent + 1);
  }
  if (node->flags & NODE_CONTAINS_RHS) {
    printf("\n");
    print_ast_node_as_sexpr(lines, node->rhs, indent + 1);
  }
  if (node->body_length) {
    printf("\n");
    for (int i = 0; i < indent; i++) printf("  ");
    printf("{");

    for (size_t i = 0; i < node->body_length; i++) {
      printf("\n");
      print_ast_node_as_sexpr(lines, &node->body[i], indent + 1);
    }

    printf("\n");
    for (int i = 0; i < indent; i++) printf("  ");
    printf("}");
  }
  printf(")");
}

void print_declaration_list_as_tree(String* lines, List* nodes) {
  for (size_t i = 0; i < nodes->length; i++) {
    AstNode* node = list_get(nodes, i);
    print_ast_node_as_tree(lines, node);
    printf("\n");
  }
}

void print_declaration_list_as_dot(String* lines, List* nodes) {
  printf("digraph G {\n");
  for (size_t i = 0; i < nodes->length; i++) {
    AstNode* node = list_get(nodes, i);
    print_ast_node_as_dot(lines, node);
    printf("\n");
  }
  printf("}\n");
}

void print_declaration_list_as_sexpr(String* lines, List* nodes) {
  printf("(\n");
  for (size_t i = 0; i < nodes->length; i++) {
    AstNode* node = list_get(nodes, i);
    print_ast_node_as_sexpr(lines, node, 1);
    printf("\n");
  }
  printf(")\n");
}

void print_typeclass(Typeclass* type) {
  // size_t id;
  // size_t size;
  // String* name;
  // List* from;
  // List* to;

  printf("<Type #%zu (%zu bits) ", type->id, type->size);
  print_string(type->name);
  printf(">");
}

int _print_bytecode(size_t* bytecode) {
  switch (bytecode[0]) {
    case BC_EXIT:
      printf("EXIT\n");
      return -1;
    case BC_LOAD:
      printf("LOAD %p\n", (void*) bytecode[1]);
      return 2;
    case BC_STORE:
      printf("STORE %p\n", (void*) bytecode[1]);
      return 2;
    case BC_ARG_LOAD:
      printf("ARG_LOAD %zu\n", (size_t) bytecode[1]);
      return 2;
    case BC_ARG_ADDR:
      printf("ARG_ADDR %zu\n", (size_t) bytecode[1]);
      return 2;
    case BC_PUSH:
      printf("PUSH %zu\n", (size_t) bytecode[1]);
      return 2;
    case BC_CALL:
      printf("CALL %p %zu\n", (void*) bytecode[1], bytecode[2]);
      return 3;
    case BC_SYSCALL:
      printf("SYSCALL %zu\n", bytecode[1]);
      return 2;
    case BC_JUMP:
      printf("JUMP %zu\n", bytecode[1]);
      return 2;
    case BC_JUMP_ZERO:
      printf("JUMP_ZERO %zu\n", bytecode[1]);
      return 2;
    default:
      printf("««%zu»»\n", bytecode[0]);
      assert(0);
  }
}

void print_bytecode(size_t* bytecode) {
  while (1) {
    int advance_by = _print_bytecode(bytecode);
    if (advance_by == -1) break;

    bytecode += advance_by;
  }
}

void print_bytecode_and_pointer(size_t* bytecode, size_t ip) {
  size_t offset = 0;
  while (1) {
    printf(ip == offset ? "==> " : "    ");
    int advance_by = _print_bytecode(bytecode + offset);
    if (advance_by == -1) break;

    offset += advance_by;
  }
}

void inspect_stack(VmState* state) {
  for (size_t i = state->sp; i != -1; i--) {
    printf("0x%zX\n", state->stack[i]);
  }
}

void inspect_vm_state(VmState* state, size_t* bytecode) {
  printf("--- VM State ---\n");
  printf("Executing Bytecode ID: %zu\n", state->id);
  printf("Frame pointer:         0x%zX\n", state->fp);
  printf("Stack pointer:         0x%zX\n", state->sp);
  printf("Instruction pointer:   0x%zX\n", state->ip);
  printf("Waiting on node:       %p\n", state->waiting_on);
  printf("\nBytecode:\n");
  print_bytecode_and_pointer(bytecode, state->ip);
  printf("\nStack:\n");
  inspect_stack(state);
  printf("\n\n");
  // typedef struct {
  //   size_t id;
  //   size_t ip;
  //   size_t sp;
  //   size_t fp;
  //
  //   size_t stack[512];
  // } VmState;
}

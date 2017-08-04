// ** Constant Operators ** //

DEFINE_STR(OP_DECLARE, ":");
DEFINE_STR(OP_DECLARE_ASSIGN, ":=");
DEFINE_STR(OP_ASSIGN, "=");
DEFINE_STR(OP_FUNC_ARROW, "=>");
DEFINE_STR(OP_OPEN_PAREN, "(");
DEFINE_STR(OP_CLOSE_PAREN, ")");
DEFINE_STR(OP_OPEN_BRACE, "{");
DEFINE_STR(OP_CLOSE_BRACE, "}");
DEFINE_STR(OP_COMMA, ",");

// ** Local Data Structures ** //

typedef struct ParserScope {
  struct ParserScope* parent;
  List* declarations;
} ParserScope;

typedef struct {
  TokenizedFile data;
  size_t pos;

  Pool* nodes;
  ParserScope* scope;
} ParserState;


void* new_parser_scope() {
  ParserScope* scope = malloc(sizeof(ParserScope));
  scope->declarations = new_list(1, 32);
  return scope;
}

void* new_parser_state(TokenizedFile* file) {
  ParserState* state = malloc(sizeof(ParserState));
  state->data = *file;
  state->pos = 0;
  state->nodes = new_pool(sizeof(AstNode), 16, 64);
  state->scope = new_parser_scope();
  return state;
}

// ** Local Debugging Tools ** //

void print_ast_node_type(AstNode* node) {
  switch (node->type) {
    case NODE_ASSIGNMENT:
      printf("ASSIGNMENT");
      break;
    // case NODE_BRANCH:
    //   printf("BRANCH");
    //   break;
    case NODE_COMPOUND:
      printf("COMPOUND");
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
      print_string(symbol_lookup(node->ident));
      printf(")");
      break;
    case NODE_TUPLE:
      printf("TUPLE(%zu)", node->body_length);
      break;
    default:
      printf("UNKNOWN");
  }
}

void print_ast_node_as_tree(ParserState* state, AstNode* node) {
  String* line = &state->data.lines[node->from.line];

  for (int i = 0; i < node->from.pos; i++) {
    printf("%c", line->data[i]);
  }
  printf("\e[0;41m");
  for (int i = node->from.pos; i < node->to.pos; i++) {
    printf("%c", line->data[i]);
  }
  printf("\e[0m");
  for (int i = node->to.pos; i < line->length; i++) {
    printf("%c", line->data[i]);
  }
  printf("  [");
  print_ast_node_type(node);
  printf("]\n");

  if (node->type == NODE_RECOVERY ||
      (node->type == NODE_EXPRESSION && node->flags == EXPR_FUNCTION)) {
    print_ast_node_as_tree(state, node->lhs);
  }

  if (node->type == NODE_DECLARATION ||
      node->type == NODE_ASSIGNMENT ||
      (node->type == NODE_EXPRESSION && node->flags == EXPR_FUNCTION)) {
    print_ast_node_as_tree(state, node->rhs);
  }

  for (int i = 0; i < node->body_length; i++) {
    print_ast_node_as_tree(state, &node->body[i]);
  }
}

// ** State Manipulation Primitives ** //

bool tokens_remain(ParserState* state) {
  return state->pos < state->data.length;
}

#define ACCEPTED (state->data.tokens[state->pos - 1])
#define TOKEN    (state->data.tokens[state->pos])

// ** Parsing Primitives ** //

int peek(ParserState* state, TokenType type) {
  return TOKEN.type == type;
}

int peek_syntax_op(ParserState* state, String* op) {
  if (TOKEN.type != TOKEN_SYNTAX_OPERATOR) return 0;
  return string_equals(op, &TOKEN.source);
}

int peek_nonsyntax_op(ParserState* state, String* op) {
  if (TOKEN.type != TOKEN_OPERATOR) return 0;
  return string_equals(op, &TOKEN.source);
}

int peek_op(ParserState* state, String* op) {
  return peek_syntax_op(state, op) || peek_nonsyntax_op(state, op);
}

int accept(ParserState* state, TokenType type) {
  if (!peek(state, type)) return 0;

  state->pos += 1;
  return 1;
}

int accept_syntax_op(ParserState* state, String* op) {
  if (!peek_syntax_op(state, op)) return 0;

  state->pos += 1;
  return 1;
}

int accept_nonsyntax_op(ParserState* state, String* op) {
  if (!peek_nonsyntax_op(state, op)) return 0;

  state->pos += 1;
  return 1;
}

int accept_op(ParserState* state, String* op) {
  if (!peek_op(state, op)) return 0;

  state->pos += 1;
  return 1;
}


// ** Lookahead Operations ** //

bool skim_tuple(ParserState* state) {
  size_t depth = 0;

  if (!peek_op(state, OP_OPEN_PAREN)) return 0;

  do {
    if (peek_op(state, OP_OPEN_PAREN)) depth += 1;
    if (peek_op(state, OP_CLOSE_PAREN)) depth -= 1;
    state->pos += 1;
  } while (tokens_remain(state) && depth > 0);

  return depth < 1;
}

bool test_declaration(ParserState* state) {
  bool result = 0;

  size_t mark = state->pos;
  if (accept(state, TOKEN_IDENTIFIER)) {
    result = peek_op(state, OP_DECLARE);
  }
  state->pos = mark;

  return result;
}

bool test_function(ParserState* state) {
  bool result = 0;

  size_t mark = state->pos;
  result = skim_tuple(state) && peek_op(state, OP_FUNC_ARROW);
  state->pos = mark;

  return result;
}


// ** Helpers ** //

void* get_node(Pool* pool, AstNodeType type) {
  AstNode* node = pool_get(pool);
  node->type = type;
  node->flags = 0;
  node->to.line = -1;
  node->to.pos = -1;
  node->body_length = 0;
  node->error = NULL;

  return node;
}


// ** Parser States ** //

void parse_declaration(ParserState* state, AstNode* decl);

// TYPE = Identifier
//      | TYPE_TUPLE "=>" TYPE          @TODO
//      | TYPE_TUPLE "=>" TYPE_TUPLE    @TODO
void parse_type(ParserState* state, AstNode* type) {
  type->to = (FileAddress) { TOKEN.line, TOKEN.pos + TOKEN.source.length };

  if (accept(state, TOKEN_IDENTIFIER)) {
    type->ident = symbol_get(&ACCEPTED.source);
  } else {
    type->error = new_string("Expected a type identifier");
  }
}

// TYPE_TUPLE = "(" ")"
//            | "(" TYPE ("," TYPE)* ")"
void parse_type_tuple(ParserState* state, AstNode* tuple) {
  Pool* pool = new_pool(sizeof(AstNode), 2, 4);

  assert(accept_op(state, OP_OPEN_PAREN));

  while (accept(state, TOKEN_IDENTIFIER)) {
    AstNode* node = get_node(pool, NODE_TYPE);
    node->from = (FileAddress) { ACCEPTED.line, ACCEPTED.pos };
    node->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
    node->ident = symbol_get(&ACCEPTED.source);

    if (!accept_op(state, OP_COMMA)) break;
  }

  tuple->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length + peek_op(state, OP_CLOSE_PAREN) };
  tuple->body_length = pool->length;
  tuple->body = pool_to_array(pool);
  free_pool(pool);

  if (!accept_op(state, OP_CLOSE_PAREN)) {
    AstNode* error = tuple;

    AstNode* temp = get_node(state->nodes, NODE_TUPLE);
    *temp = *tuple;

    size_t depth = 1;
    do {
      if (peek_op(state, OP_OPEN_PAREN)) depth += 1;
      if (peek_op(state, OP_CLOSE_PAREN)) depth -= 1;
      state->pos += 1;
    } while (tokens_remain(state) && depth > 0);

    error->type = NODE_RECOVERY;
    error->flags = 0;
    error->lhs = temp;
    error->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
    error->body_length = 0;
    error->error = new_string("Unable to parse function argument declarations");
  }
}

// DECLARATION_TUPLE = "(" ")"
//                   | "(" DECLARATION ("," DECLARATION)* ")"
void parse_declaration_tuple(ParserState* state, AstNode* tuple) {
  Pool* pool = new_pool(sizeof(AstNode), 2, 4);

  assert(accept_op(state, OP_OPEN_PAREN));

  while (test_declaration(state)) {
    AstNode* node = get_node(pool, NODE_DECLARATION);
    node->from.line = TOKEN.line;
    node->from.pos = TOKEN.pos;

    parse_declaration(state, node);

    if (!accept_op(state, OP_COMMA)) break;
  }

  tuple->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length + peek_op(state, OP_CLOSE_PAREN) };
  tuple->body_length = pool->length;
  tuple->body = pool_to_array(pool);
  free_pool(pool);

  if (!accept_op(state, OP_CLOSE_PAREN)) {
    AstNode* error = tuple;

    AstNode* temp = get_node(state->nodes, NODE_TUPLE);
    *temp = *tuple;

    size_t depth = 1;
    do {
      if (peek_op(state, OP_OPEN_PAREN)) depth += 1;
      if (peek_op(state, OP_CLOSE_PAREN)) depth -= 1;
      state->pos += 1;
    } while (tokens_remain(state) && depth > 0);

    error->type = NODE_RECOVERY;
    error->flags = 0;
    error->lhs = temp;
    error->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
    error->body_length = 0;
    error->error = new_string("Unable to parse function argument declarations");
  }
}

// CODE_BLOCK = "{" "}"
void parse_code_block(ParserState* state, AstNode* block) {
  // @TODO A lot more...
  accept_op(state, OP_OPEN_BRACE);
  accept_op(state, OP_CLOSE_BRACE);

  block->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
  block->body_length = 0;
}

// FUNCTION = DECLARATION_TUPLE "=>" TYPE_TUPLE CODE_BLOCK
//          | DECLARATION_TUPLE "=>" NAMED_TYPE_TUPLE CODE_BLOCK    @TODO
//          | DECLARATION_TUPLE "=>" TYPE CODE_BLOCK                @TODO
//          | DECLARATION_TUPLE "=>" CODE_BLOCK                     @TODO
void parse_function(ParserState* state, AstNode* func) {
  // @TODO Bubble up errors from lhs.
  func->lhs = get_node(state->nodes, NODE_TUPLE);  // Arguments
  func->lhs->from.line = TOKEN.line;
  func->lhs->from.pos = TOKEN.pos;
  parse_declaration_tuple(state, func->lhs);

  assert(accept_op(state, OP_FUNC_ARROW));

  // @TODO Bubble up errors from rhs.
  func->rhs = get_node(state->nodes, NODE_TUPLE);  // Returns
  func->rhs->from.line = TOKEN.line;
  func->rhs->from.pos = TOKEN.pos;
  parse_type_tuple(state, func->rhs);

  // @TODO Bubble up errors from rhs.
  func->body = get_node(state->nodes, NODE_COMPOUND);  // Returns
  func->body_length = 1;
  func->body->from.line = TOKEN.line;
  func->body->from.pos = TOKEN.pos;
  parse_code_block(state, func->body);
}

// EXPRESSION = Literal
//            | Identifier
//            | FUNCTION
void parse_expression(ParserState* state, AstNode* expr) {
  if (accept(state, TOKEN_LITERAL)) {
    expr->flags |= EXPR_LITERAL;
    expr->source = ACCEPTED.source;
    expr->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
  } else if (accept(state, TOKEN_IDENTIFIER)) {
    expr->flags |= EXPR_IDENT;
    expr->ident = symbol_get(&ACCEPTED.source);
    expr->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
  } else if (test_function(state)) {
    expr->flags |= EXPR_FUNCTION;
    parse_function(state, expr);
    expr->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
  } else {
    expr->error = new_string("Expected an expression");
    expr->to = (FileAddress) { TOKEN.line, TOKEN.pos + TOKEN.source.length };
  }
}

// DECLARATION = Identifier ":" TYPE
//             | Identifier ":" TYPE "=" EXPRESSION
//             | Identifier ":=" EXPRESSION            @TODO
void parse_declaration(ParserState* state, AstNode* decl) {
  {
    // The Identifier should already be guaranteed by `test_declaration`.
    assert(accept(state, TOKEN_IDENTIFIER));
    decl->ident = symbol_get(&ACCEPTED.source);
  }

  {
    // OP_DECLARE should also be guaranteed by `test_declaration`.
    assert(accept_op(state, OP_DECLARE));
  }

  {
    // Grab the TYPE.  Parse errors may reasonably occur here.
    decl->rhs = get_node(state->nodes, NODE_TYPE);
    decl->rhs->from.line = TOKEN.line;
    decl->rhs->from.pos = TOKEN.pos;
    parse_type(state, decl->rhs);

    if (decl->rhs->error != NULL) {
      decl->error = new_string("Variable declaration requires a type identifier");
    }
  }

  decl->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };

  if (accept_op(state, OP_ASSIGN)) {
    AstNode* compound = decl;
    AstNode* body = malloc(2 * sizeof(AstNode));
    body[0] = *decl;

    AstNode* value = get_node(state->nodes, NODE_EXPRESSION);
    value->from.line = TOKEN.line;
    value->from.pos = TOKEN.pos;
    parse_expression(state, value);

    // @TODO Extract a proper initializer for node data...
    AstNode* assignment = &body[1];
    assignment->type = NODE_ASSIGNMENT;
    assignment->flags = 0;
    assignment->body_length = 0;
    assignment->ident = decl->ident;
    assignment->rhs = value;
    assignment->from = decl->from;
    assignment->to = value->to;
    assignment->error = NULL;

    compound->type = NODE_COMPOUND;
    compound->flags |= COMPOUND_DECL_ASSIGN;
    compound->body_length = 2;
    compound->body = body;
    compound->from = decl->from;
    compound->to = assignment->to;
    compound->error = NULL;
  }
}

void parse_top_level_delcaration(ParserState* state, AstNode* decl) {
  parse_declaration(state, decl);

  if (!accept(state, TOKEN_NEWLINE)) {
    AstNode* error = decl;

    AstNode* temp = get_node(state->nodes, NODE_DECLARATION);
    *temp = *decl;

    // @TODO More robustly seek past the error.
    while (!peek(state, TOKEN_NEWLINE)) state->pos += 1;

    error->type = NODE_RECOVERY;
    error->flags = 0;
    error->body_length = 0;
    error->lhs = temp;
    error->to = (FileAddress) { TOKEN.line, TOKEN.pos + TOKEN.source.length };
    error->error = new_string("Unexpected code following declaration");
  }
}

//  TOP_LEVEL = DECLARATION
//            | TOP_LEVEL_DIRECTIVE    @TODO
void parse_top_level(ParserState* state) {
  while (tokens_remain(state)) {
    if (accept(state, TOKEN_NEWLINE)) {
      // Move on, nothing to see here.

    } else if (test_declaration(state)) {
      AstNode* decl = get_node(state->nodes, NODE_DECLARATION);
      decl->from.line = TOKEN.line;
      decl->from.pos = TOKEN.pos;

      parse_top_level_delcaration(state, decl);

      list_append(state->scope->declarations, decl);

    } else {
      printf("Unrecognized code in top-level context\n");
    }
  }
}


bool perform_parse_job(ParseJob* job) {
  ParserState* state = new_parser_state(job->tokens);

  parse_top_level(state);

  List* declarations = state->scope->declarations;
  for (size_t i = 0; i < declarations->length; i++) {
    AstNode* node = list_get(declarations, i);
    print_ast_node_as_tree(state, node);
    printf("\n");
  }

  return 1;
}

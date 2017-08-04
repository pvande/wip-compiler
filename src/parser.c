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

void* init_node(AstNode* node, AstNodeType type) {
  node->type = type;
  node->flags = 0;
  node->to.line = -1;
  node->to.pos = -1;
  node->body_length = 0;
  node->error = NULL;

  return node;
}


// ** Parser States ** //

AstNode* parse_declaration(ParserState* state);

// TYPE = Identifier
//      | TYPE_TUPLE "=>" TYPE          @TODO
//      | TYPE_TUPLE "=>" TYPE_TUPLE    @TODO
AstNode* parse_type(ParserState* state) {
  AstNode* type = init_node(pool_get(state->nodes), NODE_TYPE);
  type->from.line = TOKEN.line;
  type->from.pos  = TOKEN.pos;

  if (accept(state, TOKEN_IDENTIFIER)) {
    type->to.line = ACCEPTED.line;
    type->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
    type->ident   = symbol_get(&ACCEPTED.source);
  } else {
    type->to.line = TOKEN.line;
    type->to.pos  = TOKEN.pos + TOKEN.source.length;
    type->error   = new_string("Expected a type identifier");
  }

  return type;
}

// @TODO Find a way to unify this method with `parse_declaration_tuple`.
// TYPE_TUPLE = "(" ")"
//            | "(" TYPE ("," TYPE)* ")"
AstNode* parse_type_tuple(ParserState* state) {
  AstNode* tuple = init_node(pool_get(state->nodes), NODE_TUPLE);
  tuple->from.line = TOKEN.line;
  tuple->from.pos  = TOKEN.pos;

  assert(accept_op(state, OP_OPEN_PAREN));

  {
    Pool* pool = new_pool(sizeof(AstNode), 2, 4);

    while (accept(state, TOKEN_IDENTIFIER)) {
      // @TODO Find a way to unify this with `parse_type`.
      AstNode* node = init_node(pool_get(pool), NODE_TYPE);
      node->from.line = ACCEPTED.line;
      node->from.pos  = ACCEPTED.pos;
      node->to.line = ACCEPTED.line;
      node->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
      node->ident = symbol_get(&ACCEPTED.source);

      if (!accept_op(state, OP_COMMA)) break;
    }

    tuple->body_length = pool->length;
    tuple->body = pool_to_array(pool);

    free_pool(pool);
  }

  bool balanced_parens = accept_op(state, OP_CLOSE_PAREN);

  tuple->to.line = ACCEPTED.line;
  tuple->to.pos = ACCEPTED.pos + ACCEPTED.source.length;

  if (balanced_parens) return tuple;

  {
    // Error recovery
    size_t depth = 1;
    do {
      if (peek_op(state, OP_OPEN_PAREN)) depth += 1;
      if (peek_op(state, OP_CLOSE_PAREN)) depth -= 1;
      state->pos += 1;
    } while (tokens_remain(state) && depth > 0);

    AstNode* error = init_node(pool_get(state->nodes), NODE_RECOVERY);
    error->from = tuple->to;
    error->lhs = tuple;
    error->error = new_string("Unable to parse function argument declarations");

    error->to.line = ACCEPTED.line;
    error->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
    return error;
  }
}

// @TODO Find a way to unify this method with `parse_type_tuple`.
// DECLARATION_TUPLE = "(" ")"
//                   | "(" DECLARATION ("," DECLARATION)* ")"
AstNode* parse_declaration_tuple(ParserState* state) {
  AstNode* tuple = init_node(pool_get(state->nodes), NODE_TUPLE);
  tuple->from.line = TOKEN.line;
  tuple->from.pos  = TOKEN.pos;

  assert(accept_op(state, OP_OPEN_PAREN));

  {
    Pool* pool = new_pool(sizeof(AstNode), 2, 4);

    while (test_declaration(state)) {
      AstNode* node = init_node(pool_get(pool), NODE_DECLARATION);

      // @TODO Find a way to unify this with `parse_declaration`.
      // @Gross @Leak @FixMe Find a way to avoid the extra allocations here.
      *node = *parse_declaration(state);

      if (!accept_op(state, OP_COMMA)) break;
    }

    tuple->body_length = pool->length;
    tuple->body = pool_to_array(pool);

    free_pool(pool);
  }

  bool balanced_parens = accept_op(state, OP_CLOSE_PAREN);
  printf("%d\n", balanced_parens);

  tuple->to.line = ACCEPTED.line;
  tuple->to.pos = ACCEPTED.pos + ACCEPTED.source.length;

  if (balanced_parens) return tuple;

  {
    // Error recovery
    size_t depth = 1;
    do {
      if (peek_op(state, OP_OPEN_PAREN)) depth += 1;
      if (peek_op(state, OP_CLOSE_PAREN)) depth -= 1;
      state->pos += 1;
    } while (tokens_remain(state) && depth > 0);

    AstNode* error = init_node(pool_get(state->nodes), NODE_RECOVERY);
    error->from = tuple->to;
    error->lhs = tuple;
    error->error = new_string("Unable to parse function argument declarations");

    error->to.line = ACCEPTED.line;
    error->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
    print_ast_node_as_tree(state, error);
    return error;
  }
}

// CODE_BLOCK = "{" "}"
AstNode* parse_code_block(ParserState* state) {
  AstNode* block = init_node(pool_get(state->nodes), NODE_COMPOUND);
  block->from.line = TOKEN.line;
  block->from.pos  = TOKEN.pos;

  // @TODO A lot more...
  accept_op(state, OP_OPEN_BRACE);
  accept_op(state, OP_CLOSE_BRACE);

  block->body_length = 0;

  block->to.line = ACCEPTED.line;
  block->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
  return block;
}

// @TODO Bubble up errors from lhs.
// @TODO Bubble up errors from rhs.
// @TODO Bubble up errors from body.
// FUNCTION = DECLARATION_TUPLE "=>" TYPE_TUPLE CODE_BLOCK
//          | DECLARATION_TUPLE "=>" NAMED_TYPE_TUPLE CODE_BLOCK    @TODO
//          | DECLARATION_TUPLE "=>" TYPE CODE_BLOCK                @TODO
//          | DECLARATION_TUPLE "=>" CODE_BLOCK                     @TODO
AstNode* parse_function(ParserState* state) {
  AstNode* func = init_node(pool_get(state->nodes), NODE_EXPRESSION);
  func->flags = EXPR_FUNCTION;
  func->from.line = TOKEN.line;
  func->from.pos  = TOKEN.pos;

  func->lhs = parse_declaration_tuple(state);

  assert(accept_op(state, OP_FUNC_ARROW));

  func->rhs = parse_type_tuple(state);

  func->body_length = 1;
  func->body = parse_code_block(state);

  func->to.line = ACCEPTED.line;
  func->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
  return func;
}

// EXPRESSION = Literal
//            | Identifier
//            | FUNCTION
AstNode* parse_expression(ParserState* state) {
  if (accept(state, TOKEN_LITERAL)) {
    // @TODO Extract this?
    AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
    expr->from.line = TOKEN.line;
    expr->from.pos  = TOKEN.pos;

    expr->flags = EXPR_LITERAL;
    expr->source = ACCEPTED.source;

    expr->to.line = ACCEPTED.line;
    expr->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
    return expr;
  } else if (accept(state, TOKEN_IDENTIFIER)) {
    // @TODO Extract this?
    AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
    expr->from.line = TOKEN.line;
    expr->from.pos  = TOKEN.pos;

    expr->flags = EXPR_IDENT;
    expr->ident = symbol_get(&ACCEPTED.source);

    expr->to.line = ACCEPTED.line;
    expr->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;
    return expr;
  } else if (test_function(state)) {
    return parse_function(state);
  } else {
    AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
    expr->from.line = TOKEN.line;
    expr->from.pos  = TOKEN.pos;

    expr->error = new_string("Expected an expression");

    expr->to.line = TOKEN.line;
    expr->to.pos  = TOKEN.pos + TOKEN.source.length;
    return expr;
  }
}

// DECLARATION = Identifier ":" TYPE
//             | Identifier ":" TYPE "=" EXPRESSION
//             | Identifier ":=" EXPRESSION            @TODO
AstNode* parse_declaration(ParserState* state) {
  AstNode* decl = init_node(pool_get(state->nodes), NODE_DECLARATION);
  decl->from.line = TOKEN.line;
  decl->from.pos  = TOKEN.pos;

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
    decl->rhs = parse_type(state);

    if (decl->rhs->error != NULL) {
      decl->error = new_string("Variable declaration requires a type identifier");
    }
  }

  decl->to.line = ACCEPTED.line;
  decl->to.pos  = ACCEPTED.pos + ACCEPTED.source.length;

  if (!accept_op(state, OP_ASSIGN)) return decl;

  // Since we already have a pool-allocated node for the declaration, but we
  // need two contiguous nodes for the COMPOUND node we're building, we'll do
  // some clever work and swap the two.  If we allocate the COMPOUND body, then
  // we'll have space to copy the declaration data; that will free up the `decl`
  // node to be re-initialized and used as the memory for the COMPOUND node.
  // All in all, this works, but there's probably a cleaner way to structure all
  // of this.  This may also have to be rewritten when we implement the untyped
  // decl/assign operator.
  //     - pvande, Aug 4, 2017
  AstNode* body = malloc(2 * sizeof(AstNode));
  body[0] = *decl;

  AstNode* assignment = init_node(&body[1], NODE_ASSIGNMENT);
  assignment->from.line = TOKEN.line;
  assignment->from.pos  = TOKEN.pos;

  // @TODO Find a way to unify this with `parse_expression`.
  // @Gross @Leak @FixMe Find a way to avoid the extra allocations here.
  AstNode* expr = parse_expression(state);
  *assignment = *expr;

  // @TODO Bubble up errors.
  AstNode* compound = init_node(decl, NODE_COMPOUND);
  compound->flags = COMPOUND_DECL_ASSIGN;
  compound->from = body[0].from;
  compound->to = body[1].to;
  compound->body_length = 2;
  compound->body = body;

  return compound;
}

AstNode* parse_top_level_delcaration(ParserState* state) {
  AstNode* decl = parse_declaration(state);

  if (accept(state, TOKEN_NEWLINE)) return decl;

  {
    // Error recovery
    AstNode* error = init_node(decl, NODE_RECOVERY);
    error->from.line = TOKEN.line;
    error->from.pos  = TOKEN.pos;
    error->lhs = decl;
    error->error = new_string("Unexpected code following declaration");

    // @TODO More robustly seek past the error.
    while (!peek(state, TOKEN_NEWLINE)) state->pos += 1;

    error->to.line = TOKEN.line;
    error->to.pos  = TOKEN.pos + TOKEN.source.length;
    return error;
  }
}

//  TOP_LEVEL = DECLARATION
//            | TOP_LEVEL_DIRECTIVE    @TODO
bool perform_parse_job(ParseJob* job) {
  ParserState* state = new_parser_state(job->tokens);

  while (tokens_remain(state)) {
    if (accept(state, TOKEN_NEWLINE)) {
      // Move on, nothing to see here.

    } else if (test_declaration(state)) {
      AstNode* decl = parse_top_level_delcaration(state);
      // list_append(state->scope->declarations, decl);
      print_ast_node_as_tree(state, decl);

    } else {
      printf("Unrecognized code in top-level context\n");
    }
  }

  // List* declarations = state->scope->declarations;
  // for (size_t i = 0; i < declarations->length; i++) {
  //   AstNode* node = list_get(declarations, i);
  //   print_ast_node_as_tree(state, node);
  //   printf("\n");
  // }

  return 1;
}

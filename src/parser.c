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

// ** Constant Strings ** //

DEFINE_STR(STR_VOID, "void");

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
  if (node == NULL) return;

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
      node->type == NODE_ASSIGNMENT ||
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

FileAddress token_start(Token t) {
  return (FileAddress) { t.line, t.pos };
}

FileAddress token_end(Token t) {
  return (FileAddress) { t.line, t.pos + t.source.length };
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

bool test_type(ParserState* state) {
  return peek(state, TOKEN_IDENTIFIER);
}

bool test_declaration(ParserState* state) {
  bool result = 0;

  size_t mark = state->pos;
  if (accept(state, TOKEN_IDENTIFIER)) {
    result = peek_op(state, OP_DECLARE) || peek_op(state, OP_DECLARE_ASSIGN);
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

AstNode* _parse_tuple(ParserState* state, bool (*more)(ParserState* state), void (*parse_node)(ParserState*, AstNode*)) {
  AstNode* tuple = init_node(pool_get(state->nodes), NODE_TUPLE);
  tuple->from = token_start(TOKEN);

  assert(accept_op(state, OP_OPEN_PAREN));

  {
    Pool* pool = new_pool(sizeof(AstNode), 2, 4);

    while (more(state)) {
      AstNode* node = pool_get(pool);
      parse_node(state, node);

      if (!accept_op(state, OP_COMMA)) break;
    }

    tuple->body_length = pool->length;
    tuple->body = pool_to_array(pool);

    free_pool(pool);
  }

  bool balanced_parens = accept_op(state, OP_CLOSE_PAREN);

  tuple->to = token_end(ACCEPTED);

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
    error->to = token_end(ACCEPTED);
    error->lhs = tuple;
    error->error = new_string("Unable to parse function argument declarations");

    return error;
  }
}

// ** Parser States ** //

AstNode* parse_declaration(ParserState* state);

// TYPE = Identifier
//      | TYPE_TUPLE "=>" TYPE          @TODO
//      | TYPE_TUPLE "=>" TYPE_TUPLE    @TODO
AstNode* parse_type(ParserState* state) {
  AstNode* type = init_node(pool_get(state->nodes), NODE_TYPE);
  type->from = token_start(TOKEN);

  if (accept(state, TOKEN_IDENTIFIER)) {
    type->ident = symbol_get(&ACCEPTED.source);
    type->to = token_end(ACCEPTED);
  } else {
    type->error = new_string("Expected a type identifier");
    type->to = token_end(TOKEN);
  }

  return type;
}

// @TODO Find a way to unify this with `parse_type`.
void _parse_type(ParserState* state, AstNode* node) {
  accept(state, TOKEN_IDENTIFIER);

  init_node(node, NODE_TYPE);
  node->from = token_start(ACCEPTED);
  node->to = token_end(ACCEPTED);
  node->ident = symbol_get(&ACCEPTED.source);
}

// TYPE_TUPLE = "(" ")"
//            | "(" TYPE ("," TYPE)* ")"
AstNode* parse_type_tuple(ParserState* state) {
  return _parse_tuple(state, test_type, _parse_type);
}

// @TODO Find a way to actually unify this with `parse_declaration`.
void _parse_declaration(ParserState* state, AstNode* node) {
  init_node(node, NODE_DECLARATION);

  // @Gross @Leak @FixMe Find a way to avoid the extra allocations here.
  *node = *parse_declaration(state);
}

// DECLARATION_TUPLE = "(" ")"
//                   | "(" DECLARATION ("," DECLARATION)* ")"
AstNode* parse_declaration_tuple(ParserState* state) {
  return _parse_tuple(state, test_declaration, _parse_declaration);
}

// CODE_BLOCK = "{" "}"
AstNode* parse_code_block(ParserState* state) {
  AstNode* block = init_node(pool_get(state->nodes), NODE_COMPOUND);
  block->from = token_start(TOKEN);

  // @TODO A lot more...
  accept_op(state, OP_OPEN_BRACE);
  accept_op(state, OP_CLOSE_BRACE);

  block->body_length = 0;

  block->to = token_end(ACCEPTED);
  return block;
}

// @TODO Bubble up errors from lhs.
// @TODO Bubble up errors from rhs.
// @TODO Bubble up errors from body.
// FUNCTION = DECLARATION_TUPLE "=>" CODE_BLOCK
//          | DECLARATION_TUPLE "=>" TYPE CODE_BLOCK
//          | DECLARATION_TUPLE "=>" TYPE_TUPLE CODE_BLOCK
//          | DECLARATION_TUPLE "=>" NAMED_TYPE_TUPLE CODE_BLOCK    @TODO
//          | DECLARATION_TUPLE "=>" TYPE CODE_BLOCK                @TODO
//          | DECLARATION_TUPLE "=>" CODE_BLOCK                     @TODO
AstNode* parse_function(ParserState* state) {
  AstNode* func = init_node(pool_get(state->nodes), NODE_EXPRESSION);
  func->flags = EXPR_FUNCTION;
  func->from = token_start(TOKEN);

  func->lhs = parse_declaration_tuple(state);

  assert(accept_op(state, OP_FUNC_ARROW));

  if (peek_op(state, OP_OPEN_PAREN)) {
    func->rhs = parse_type_tuple(state);
    
  } else if (peek_op(state, OP_OPEN_BRACE)) {
    AstNode* type = init_node(pool_get(state->nodes), NODE_TYPE);
    type->from = token_start(TOKEN);
    type->to = token_start(TOKEN);
    type->ident = symbol_get(STR_VOID);

    AstNode* tuple = init_node(pool_get(state->nodes), NODE_TUPLE);
    tuple->from = type->from;
    tuple->to = type->to;
    tuple->body_length = 1;
    tuple->body = type;

    func->rhs = tuple;

  } else if (test_type(state)) {
    AstNode* type = parse_type(state);

    AstNode* tuple = init_node(pool_get(state->nodes), NODE_TUPLE);
    tuple->from = type->from;
    tuple->to = type->to;
    tuple->body_length = 1;
    tuple->body = type;

    func->rhs = tuple;

  } else {
    // @TODO Actually recover from this error case.
    assert(0);
  }

  func->body_length = 1;
  func->body = parse_code_block(state);

  func->to = token_end(ACCEPTED);
  return func;
}

// EXPRESSION = Literal
//            | Identifier
//            | FUNCTION
AstNode* parse_expression(ParserState* state) {
  if (accept(state, TOKEN_LITERAL)) {
    // @TODO Extract this?
    AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
    expr->flags = EXPR_LITERAL;
    expr->from = token_start(ACCEPTED);
    expr->to = token_end(ACCEPTED);
    expr->source = ACCEPTED.source;
    return expr;

  } else if (accept(state, TOKEN_IDENTIFIER)) {
    // @TODO Extract this?
    AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
    expr->flags = EXPR_IDENT;
    expr->from = token_start(ACCEPTED);
    expr->to = token_end(ACCEPTED);
    expr->ident = symbol_get(&ACCEPTED.source);
    return expr;

  } else if (test_function(state)) {
    return parse_function(state);

  } else {
    AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
    expr->from = token_start(TOKEN);
    expr->to = token_end(TOKEN);
    expr->error = new_string("Expected an expression");
    return expr;
  }
}

// DECLARATION = Identifier ":" TYPE
//             | Identifier ":" TYPE "=" EXPRESSION
//             | Identifier ":=" EXPRESSION
AstNode* parse_declaration(ParserState* state) {
  FileAddress decl_start = token_start(TOKEN);
  Symbol name;
  AstNode* type = NULL;
  AstNode* value = NULL;

  {
    // The Identifier should already be guaranteed by `test_declaration`.
    assert(accept(state, TOKEN_IDENTIFIER));
    name = symbol_get(&ACCEPTED.source);
  }

  if (accept_op(state, OP_DECLARE)) {
    type = parse_type(state);
  } else {
    assert(accept_op(state, OP_DECLARE_ASSIGN));
  }

  if (type == NULL || accept_op(state, OP_ASSIGN)) {
    value = parse_expression(state);

    AstNode* decl = init_node(pool_get(state->nodes), NODE_DECLARATION);
    decl->from = decl_start;
    decl->to = token_end(ACCEPTED);
    decl->ident = name;
    decl->rhs = type;

    AstNode* assignment = init_node(pool_get(state->nodes), NODE_ASSIGNMENT);
    assignment->from = decl_start;
    assignment->to = token_end(ACCEPTED);
    assignment->lhs = decl;
    assignment->rhs = value;

    // @TODO Bubble up errors.
    return assignment;
  } else {
    AstNode* decl = init_node(pool_get(state->nodes), NODE_DECLARATION);
    decl->from = decl_start;
    decl->to = token_end(ACCEPTED);
    decl->ident = name;
    decl->rhs = type;

    // @TODO Bubble up errors.
    return decl;
  }
}

AstNode* parse_top_level_delcaration(ParserState* state) {
  AstNode* decl = parse_declaration(state);

  if (accept(state, TOKEN_NEWLINE)) return decl;

  {
    // Error recovery
    AstNode* error = init_node(pool_get(state->nodes), NODE_RECOVERY);
    error->from = token_start(TOKEN);
    error->lhs = decl;
    error->error = new_string("Unexpected code following declaration");

    // @TODO More robustly seek past the error.
    while (!peek(state, TOKEN_NEWLINE)) state->pos += 1;

    error->to = token_end(TOKEN);
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
      list_append(state->scope->declarations, decl);
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

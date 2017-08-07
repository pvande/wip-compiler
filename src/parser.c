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
DEFINE_STR(OP_NEWLINE, "\n");

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

bool test_not_end_of_tuple(ParserState* state) {
  return !peek_op(state, OP_CLOSE_PAREN);
}

bool test_not_end_of_block(ParserState* state) {
  return !peek_op(state, OP_CLOSE_BRACE);
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
  static size_t serial = 0;
  node->type = type;
  node->flags = 0;
  node->id = serial++;
  node->to.line = -1;
  node->to.pos = -1;
  node->body_length = 0;
  node->error = NULL;

  return node;
}

AstNode* _parse_tuple(ParserState* state,
                      String* open_operator,
                      String* close_operator,
                      String* separator,
                      bool (*more)(ParserState* state),
                      void (*parse_node)(ParserState*, AstNode*)) {
  AstNode* tuple = init_node(pool_get(state->nodes), NODE_COMPOUND);
  tuple->from = token_start(TOKEN);

  assert(accept_op(state, open_operator));
  while (accept_op(state, OP_NEWLINE)) {}

  {
    Pool* pool = new_pool(sizeof(AstNode), 2, 4);

    while (more(state)) {
      AstNode* node = pool_get(pool);
      parse_node(state, node);

      if (separator != OP_NEWLINE) {
        while (accept_op(state, OP_NEWLINE)) {}
      }

      if (!accept_op(state, separator)) break;
      while (accept_op(state, OP_NEWLINE)) {}
    }

    tuple->body_length = pool->length;
    tuple->body = pool_to_array(pool);

    free_pool(pool);
  }

  bool properly_balanced = accept_op(state, close_operator);

  tuple->to = token_end(ACCEPTED);

  if (properly_balanced) return tuple;

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
AstNode* parse_expression(ParserState* state);

// TYPE = Identifier
//      | TYPE_TUPLE "=>" TYPE          @TODO
//      | TYPE_TUPLE "=>" TYPE_TUPLE    @TODO
AstNode* parse_type(ParserState* state) {
  AstNode* type = init_node(pool_get(state->nodes), NODE_TYPE);
  type->from = token_start(TOKEN);

  if (accept(state, TOKEN_IDENTIFIER)) {
    type->source = ACCEPTED.source;
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
  node->source = ACCEPTED.source;
}

// TYPE_TUPLE = "(" ")"
//            | "(" TYPE ("," TYPE)* ")"
AstNode* parse_type_tuple(ParserState* state) {
  return _parse_tuple(state, OP_OPEN_PAREN, OP_CLOSE_PAREN, OP_COMMA, test_type, _parse_type);
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
  return _parse_tuple(state, OP_OPEN_PAREN, OP_CLOSE_PAREN, OP_COMMA, test_declaration, _parse_declaration);
}

// @TODO Find a way to actually unify this with `parse_declaration`.
void _parse_expression(ParserState* state, AstNode* node) {
  init_node(node, NODE_EXPRESSION);

  // @Gross @Leak @FixMe Find a way to avoid the extra allocations here.
  *node = *parse_expression(state);
}

// EXPRESSION_TUPLE = "(" ")"
//                   | "(" EXPRESSION ("," EXPRESSION)* ")"
AstNode* parse_expression_tuple(ParserState* state) {
  return _parse_tuple(state, OP_OPEN_PAREN, OP_CLOSE_PAREN, OP_COMMA, test_not_end_of_tuple, _parse_expression);
}

void _parse_statement(ParserState* state, AstNode* node) {
  if (accept_op(state, OP_NEWLINE)) {
    // Move on, nothing to see here.

  } else if (test_declaration(state)) {
    _parse_declaration(state, node);

  } else {
    _parse_expression(state, node);
  }
}

// CODE_BLOCK = "{" "}"
AstNode* parse_code_block(ParserState* state) {
  return _parse_tuple(state, OP_OPEN_BRACE, OP_CLOSE_BRACE, OP_NEWLINE, test_not_end_of_block, _parse_statement);
}

// @TODO Bubble up errors from lhs.
// @TODO Bubble up errors from rhs.
// @TODO Bubble up errors from body.
// FUNCTION = DECLARATION_TUPLE "=>" CODE_BLOCK
//          | DECLARATION_TUPLE "=>" TYPE CODE_BLOCK
//          | DECLARATION_TUPLE "=>" TYPE_TUPLE CODE_BLOCK
//          | DECLARATION_TUPLE "=>" NAMED_TYPE_TUPLE CODE_BLOCK    @TODO
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
    type->source = *STR_VOID;

    AstNode* tuple = init_node(pool_get(state->nodes), NODE_COMPOUND);
    tuple->from = type->from;
    tuple->to = type->to;
    tuple->body_length = 1;
    tuple->body = type;

    func->rhs = tuple;

  } else if (test_type(state)) {
    AstNode* type = parse_type(state);

    AstNode* tuple = init_node(pool_get(state->nodes), NODE_COMPOUND);
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
//            | Identifier EXPRESSION_TUPLE
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
    Symbol name = symbol_get(&ACCEPTED.source);
    FileAddress start = token_start(ACCEPTED);

    if (peek_op(state, OP_OPEN_PAREN)) {
      AstNode* arguments = parse_expression_tuple(state);

      AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
      expr->flags = EXPR_CALL;
      expr->from = start;
      expr->to = token_end(ACCEPTED);
      expr->ident = name;
      expr->rhs = arguments;
      return expr;

    } else {
      // @TODO Extract this?
      AstNode* expr = init_node(pool_get(state->nodes), NODE_EXPRESSION);
      expr->flags = EXPR_IDENT;
      expr->from = start;
      expr->to = token_end(ACCEPTED);
      expr->ident = name;
      return expr;
    }

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

    if (type != NULL) {
      decl->flags |= DECL_TYPE_PROVIDED;
      decl->typeclass = type;
    }

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

  if (accept_op(state, OP_NEWLINE)) return decl;

  {
    // Error recovery
    AstNode* error = init_node(pool_get(state->nodes), NODE_RECOVERY);
    error->from = token_start(TOKEN);
    error->lhs = decl;
    error->error = new_string("Unexpected code following declaration");

    // @TODO More robustly seek past the error.
    while (!peek_op(state, OP_NEWLINE)) state->pos += 1;

    error->to = token_end(ACCEPTED);
    return error;
  }
}

//  TOP_LEVEL = DECLARATION
//            | TOP_LEVEL_DIRECTIVE    @TODO
bool perform_parse_job(ParseJob* job) {
  ParserState* state = new_parser_state(job->tokens);

  while (tokens_remain(state)) {
    if (accept_op(state, OP_NEWLINE)) {
      // Move on, nothing to see here.

    } else if (test_declaration(state)) {
      AstNode* decl = parse_top_level_delcaration(state);
      list_append(state->scope->declarations, decl);

      if (decl->error == NULL) {
        pipeline_emit_typecheck_job(decl);
      } else {
        // @TODO Define an error reporting job, and dispatch the `decl` to that.
      }

      // print_ast_node_as_tree(state, decl);

    } else {
      printf("Unrecognized code in top-level context\n");
    }
  }

  print_declaration_list_as_dot(state->data.lines, state->scope->declarations);

  return 1;
}

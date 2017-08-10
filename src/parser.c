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

// ** Local Data Structures ** //

typedef struct {
  TokenizedFile data;
  size_t pos;

  Pool* nodes;
  Scope* scope;
} ParserState;


void* new_parser_scope(Scope* parent) {
  Scope* scope = malloc(sizeof(Scope));
  scope->declarations = new_list(1, 32);
  scope->parent = parent;
  return scope;
}

void* new_parser_state(TokenizedFile* file) {
  ParserState* state = malloc(sizeof(ParserState));
  state->data = *file;
  state->pos = 0;
  state->nodes = new_pool(sizeof(AstNode), 16, 64);
  state->scope = new_parser_scope(NULL);
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

bool test_assignment(ParserState* state) {
  bool result = 0;

  size_t mark = state->pos;
  if (accept(state, TOKEN_IDENTIFIER)) {
    result = peek_op(state, OP_ASSIGN) || peek_op(state, OP_DECLARE_ASSIGN);
  }
  state->pos = mark;

  return result;
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

bool test_procedure(ParserState* state) {
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
  node->typeclass = NULL;
  node->typekind = 0;
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
    error->error = new_string("Unable to parse procedure argument declarations");

    return error;
  }
}

// ** Parser States ** //

AstNode* parse_type(ParserState* state);
AstNode* parse_type_tuple(ParserState* state);
AstNode* parse_declaration_tuple(ParserState* state);
AstNode* parse_expression_tuple(ParserState* state);
AstNode* parse_code_block(ParserState* state);
AstNode* parse_procedure(ParserState* state);
AstNode* parse_expression(ParserState* state);
AstNode* parse_declaration(ParserState* state);
AstNode* parse_assignment(ParserState* state);


void parse_type_node(ParserState* state, AstNode* node) {
  init_node(node, NODE_TYPE);

  node->from = token_start(TOKEN);

  if (accept(state, TOKEN_IDENTIFIER)) {
    node->source = ACCEPTED.source;
    node->to = token_end(ACCEPTED);
  } else {
    node->error = new_string("Expected a type identifier");
    node->to = token_end(TOKEN);
  }
}

void parse_procedure_node(ParserState* state, AstNode* proc) {
  // Initialized in `parse_expression_node`.

  proc->flags = EXPR_PROCEDURE;
  proc->from = token_start(TOKEN);

  // "Push" a new scope onto the stack.
  state->scope = new_parser_scope(state->scope);

  proc->lhs = parse_declaration_tuple(state);

  assert(accept_op(state, OP_FUNC_ARROW));

  if (peek_op(state, OP_OPEN_PAREN)) {
    proc->rhs = parse_type_tuple(state);

  } else if (peek_op(state, OP_OPEN_BRACE)) {
    AstNode* tuple = init_node(pool_get(state->nodes), NODE_COMPOUND);
    tuple->from = token_start(TOKEN);
    tuple->to = token_start(TOKEN);
    tuple->body_length = 0;

    proc->rhs = tuple;

  } else if (test_type(state)) {
    AstNode* type = parse_type(state);

    AstNode* tuple = init_node(pool_get(state->nodes), NODE_COMPOUND);
    tuple->from = type->from;
    tuple->to = type->to;
    tuple->body_length = 1;
    tuple->body = type;

    proc->rhs = tuple;

  } else {
    // @TODO Actually recover from this error case.
    assert(0);
  }


  proc->body_length = 1;
  proc->body = parse_code_block(state);
  proc->body->scope = state->scope;

  proc->to = token_end(ACCEPTED);

  // "Pop" the scope off the stack.
  state->scope = state->scope->parent;
}

void parse_declaration_node(ParserState* state, AstNode* decl) {
  init_node(decl, NODE_DECLARATION);

  decl->from = token_start(TOKEN);

  {
    // The Identifier should already be guaranteed by `test_declaration`.
    assert(accept(state, TOKEN_IDENTIFIER));
    decl->ident = symbol_get(&ACCEPTED.source);
  }

  if (accept_op(state, OP_DECLARE)) {
    decl->rhs = parse_type(state);
  } else {
    assert(peek_op(state, OP_DECLARE_ASSIGN));
  }

  decl->to = token_end(ACCEPTED);

  // @TODO Bubble up errors.
}

void parse_expression_node(ParserState* state, AstNode* expr) {
  init_node(expr, NODE_EXPRESSION);

  if (accept(state, TOKEN_LITERAL)) {
    // @TODO Extract this?
    expr->flags = EXPR_LITERAL | ACCEPTED.literal_type;
    expr->from = token_start(ACCEPTED);
    expr->to = token_end(ACCEPTED);
    expr->source = ACCEPTED.source;

  } else if (accept(state, TOKEN_IDENTIFIER)) {
    Symbol name = symbol_get(&ACCEPTED.source);
    FileAddress start = token_start(ACCEPTED);

    if (peek_op(state, OP_OPEN_PAREN)) {
      AstNode* arguments = parse_expression_tuple(state);

      expr->flags = EXPR_CALL;
      expr->from = start;
      expr->to = token_end(ACCEPTED);
      expr->ident = name;
      expr->rhs = arguments;
      expr->scope = state->scope;

    } else {
      expr->flags = EXPR_IDENT;
      expr->from = start;
      expr->to = token_end(ACCEPTED);
      expr->ident = name;
      expr->scope = state->scope;
    }

  } else if (test_procedure(state)) {
    parse_procedure_node(state, expr);

  } else {
    expr->from = token_start(TOKEN);
    expr->to = token_end(TOKEN);
    expr->error = new_string("Expected an expression");
  }
}

void parse_assignment_node(ParserState* state, AstNode* node) {
  init_node(node, NODE_ASSIGNMENT);

  // @Lazy Use a better test.
  if (test_declaration(state)) {
    AstNode* decl = parse_declaration(state);
    accept_op(state, OP_DECLARE_ASSIGN);
    AstNode* value = parse_expression(state);

    node->from = decl->from;
    node->to = token_end(ACCEPTED);
    node->lhs = decl;
    node->rhs = value;

    // @TODO Bubble up errors.

  } else {
    // Arbitrary assignment expressions aren't yet handled.
    assert(0);
  }
}

// TYPE = Identifier
//      | TYPE_TUPLE "=>" TYPE          @TODO
//      | TYPE_TUPLE "=>" TYPE_TUPLE    @TODO
AstNode* parse_type(ParserState* state) {
  AstNode* type = pool_get(state->nodes);
  parse_type_node(state, type);
  return type;
}

// TYPE_TUPLE = "(" ")"
//            | "(" TYPE ("," TYPE)* ")"
AstNode* parse_type_tuple(ParserState* state) {
  return _parse_tuple(state, OP_OPEN_PAREN, OP_CLOSE_PAREN, OP_COMMA, test_type, parse_type_node);
}

// DECLARATION_TUPLE = "(" ")"
//                   | "(" DECLARATION ("," DECLARATION)* ")"
AstNode* parse_declaration_tuple(ParserState* state) {
  AstNode* tuple = _parse_tuple(state, OP_OPEN_PAREN, OP_CLOSE_PAREN, OP_COMMA, test_declaration, parse_declaration_node);

  // We have to do this insane juggling here, because we can't rely on the
  // tuple's pool-allocated node pointers being stable.
  for (size_t i = 0; i < tuple->body_length; i++) {
    AstNode* node = &tuple->body[i];
    if (node->type == NODE_ASSIGNMENT) node = node->lhs;
    if (node->type == NODE_DECLARATION) list_append(state->scope->declarations, node);
  }

  return tuple;
}

// EXPRESSION_TUPLE = "(" ")"
//                   | "(" EXPRESSION ("," EXPRESSION)* ")"
AstNode* parse_expression_tuple(ParserState* state) {
  return _parse_tuple(state, OP_OPEN_PAREN, OP_CLOSE_PAREN, OP_COMMA, test_not_end_of_tuple, parse_expression_node);
}

// STATEMENT = DECLARATION
//           | EXPRESSION
void parse_statement_node(ParserState* state, AstNode* node) {
  if (accept_op(state, OP_NEWLINE)) {
    assert(0);  // This should always be managed by the _parse_tuple method.

  } else if (test_assignment(state)) {
    parse_assignment_node(state, node);

  } else if (test_declaration(state)) {
    parse_declaration_node(state, node);

  } else {
    parse_expression_node(state, node);
  }
}

// @Precondition A block-local scope has been pushed onto the scope stack.
// CODE_BLOCK = "{" "}"
//            | "(" STATEMENT ("," STATEMENT)* ")"
AstNode* parse_code_block(ParserState* state) {
  AstNode* block = _parse_tuple(state, OP_OPEN_BRACE, OP_CLOSE_BRACE, OP_NEWLINE, test_not_end_of_block, parse_statement_node);

  // We have to do this insane juggling here, because we can't rely on the
  // tuple's pool-allocated node pointers being stable.
  for (size_t i = 0; i < block->body_length; i++) {
    AstNode* node = &block->body[i];
    if (node->type == NODE_ASSIGNMENT) node = node->lhs;
    if (node->type == NODE_DECLARATION) list_append(state->scope->declarations, node);
  }

  return block;
}

// @TODO Bubble up errors from lhs.
// @TODO Bubble up errors from rhs.
// @TODO Bubble up errors from body.
// PROCEDURE = DECLARATION_TUPLE "=>" CODE_BLOCK
//           | DECLARATION_TUPLE "=>" TYPE CODE_BLOCK
//           | DECLARATION_TUPLE "=>" TYPE_TUPLE CODE_BLOCK
//           | DECLARATION_TUPLE "=>" NAMED_TYPE_TUPLE CODE_BLOCK    @TODO
AstNode* parse_procedure(ParserState* state) {
  AstNode* proc = pool_get(state->nodes);
  parse_procedure_node(state, proc);
  return proc;
}

// EXPRESSION = Literal
//            | Identifier EXPRESSION_TUPLE
//            | Identifier
//            | PROCEDURE
AstNode* parse_expression(ParserState* state) {
  AstNode* expr = pool_get(state->nodes);
  parse_expression_node(state, expr);
  return expr;
}

// DECLARATION = Identifier ":" TYPE
//             | Identifier ":" TYPE "=" EXPRESSION
//             | Identifier ":=" EXPRESSION
AstNode* parse_declaration(ParserState* state) {
  AstNode* decl = pool_get(state->nodes);
  parse_declaration_node(state, decl);

  list_append(state->scope->declarations, decl);

  return decl;
}

// ASSIGNMENT = Identifier "=" EXPRESSION
//            | DECLARATION ":=" EXPRESSION
AstNode* parse_assignment(ParserState* state) {
  AstNode* assignment = pool_get(state->nodes);
  parse_assignment_node(state, assignment);
  return assignment;
}

//  TOP_LEVEL = DECLARATION
//            | ASSIGNMENT
//            | TOP_LEVEL_DIRECTIVE    @TODO
AstNode* parse_top_level(ParserState* state) {
  AstNode* node;

  if (test_assignment(state)) {
    node = parse_assignment(state);
  } else if (test_declaration(state)) {
    node = parse_declaration(state);
  } else {
    printf("Invalid top-level expression on line %zu!", TOKEN.line);
    assert(0);
  }

  if (accept_op(state, OP_NEWLINE)) return node;

  {
    // Error recovery
    AstNode* error = init_node(pool_get(state->nodes), NODE_RECOVERY);
    error->from = token_start(TOKEN);
    error->lhs = node;
    error->error = new_string("Unexpected code following declaration");

    // @TODO More robustly seek past the error.
    while (!peek_op(state, OP_NEWLINE)) state->pos += 1;

    error->to = token_end(ACCEPTED);
    return error;
  }
}

bool perform_parse_job(ParseJob* job) {
  ParserState* state = new_parser_state(job->tokens);

  while (tokens_remain(state)) {
    if (accept_op(state, OP_NEWLINE)) {
      // Move on, nothing to see here.

    } else {
      AstNode* node = parse_top_level(state);

      if (node->error == NULL) {
        pipeline_emit_typecheck_job(node);
      } else {
        // @TODO Define an error reporting job, and dispatch the `decl` to that.
      }

      // print_ast_node_as_tree(state->data.lines, node);
    }
  }

  // print_declaration_list_as_tree(state->data.lines, state->scope->declarations);

  return 1;
}

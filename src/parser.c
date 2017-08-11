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

// ** Constant Errors ** //

// const char* COMPILER_ERROR = "Internal Compiler Error: %s";
DEFINE_STR(ERR_EXPECTED_TYPE, "Expected a type");
DEFINE_STR(ERR_EXPECTED_EXPRESSION, "Expected an expression");
DEFINE_STR(ERR_EXPECTED_EOL, "Unexpected code following statement");
DEFINE_STR(ERR_EXPECTED_CLOSE, "Unexpected code in argument list");
DEFINE_STR(ERR_UNCLOSED_STRING, "String literal is unterminated");

// ** Local Data Structures ** //

typedef struct {
  Token* tokens;
  size_t length;
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
  state->tokens = file->tokens;
  state->length = file->length;
  state->pos = 0;
  state->nodes = new_pool(sizeof(AstNode), 16, 64);
  state->scope = new_parser_scope(NULL);
  return state;
}


// ** State Manipulation Primitives ** //

bool tokens_remain(ParserState* state) {
  return state->pos < state->length;
}

FileAddress token_start(Token t) {
  return (FileAddress) { t.line, t.pos };
}

FileAddress token_end(Token t) {
  return (FileAddress) { t.line, t.pos + t.source.length };
}

#define ACCEPTED (state->tokens[state->pos - 1])
#define TOKEN    (state->tokens[state->pos])


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
    result |= peek_op(state, OP_DECLARE_ASSIGN);
    result |= peek_op(state, OP_ASSIGN);
    result |= accept_op(state, OP_DECLARE) && accept(state, TOKEN_IDENTIFIER) && peek_op(state, OP_ASSIGN);
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

  bool parse_errors = 0;

  {
    Pool* pool = new_pool(sizeof(AstNode), 2, 4);

    while (more(state)) {
      AstNode* node = pool_get(pool);
      parse_node(state, node);
      if (node->flags & NODE_CONTAINS_ERROR) parse_errors += 1;

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

  if (parse_errors) tuple->flags |= NODE_CONTAINS_ERROR;

  if (!properly_balanced) {
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
    error->error = ERR_EXPECTED_CLOSE; // @TODO: Parameterize?
    error->flags |= NODE_CONTAINS_LHS;
    error->flags |= NODE_CONTAINS_ERROR;

    return error;
  }

  return tuple;
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
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_EXPECTED_TYPE;
    node->to = token_end(TOKEN);
  }
}

void parse_procedure_node(ParserState* state, AstNode* node) {
  // Initialized in `parse_expression_node`.

  node->flags = EXPR_PROCEDURE;
  node->from = token_start(TOKEN);

  // "Push" a new scope onto the stack.
  state->scope = new_parser_scope(state->scope);

  node->lhs = parse_declaration_tuple(state);

  assert(accept_op(state, OP_FUNC_ARROW));

  if (peek_op(state, OP_OPEN_PAREN)) {
    node->rhs = parse_type_tuple(state);

  } else if (peek_op(state, OP_OPEN_BRACE)) {
    node->rhs = init_node(pool_get(state->nodes), NODE_COMPOUND);
    node->rhs->from = token_start(TOKEN);
    node->rhs->to = token_start(TOKEN);
    node->rhs->body_length = 0;

  } else if (test_type(state)) {
    AstNode* type = parse_type(state);

    // `test_type` should be guaranteeing a usable `type` node here.
    assert(!(type->flags & NODE_CONTAINS_ERROR));

    node->rhs = init_node(pool_get(state->nodes), NODE_COMPOUND);
    node->rhs->from = type->from;
    node->rhs->to = type->to;
    node->rhs->body_length = 1;
    node->rhs->body = type;

  } else {
    // @TODO Actually recover from this error case.
    assert(0);
  }

  node->body_length = 1;
  node->body = parse_code_block(state);
  node->body->scope = state->scope;
  node->to = token_end(ACCEPTED);
  node->flags |= NODE_CONTAINS_LHS;
  node->flags |= NODE_CONTAINS_RHS;
  node->flags |= (node->lhs->flags & NODE_CONTAINS_ERROR);
  node->flags |= (node->rhs->flags & NODE_CONTAINS_ERROR);
  node->flags |= (node->body->flags & NODE_CONTAINS_ERROR);

  // @UX Specialize error message if error is found in `lhs` or `rhs`.

  // "Pop" the scope off the stack.
  state->scope = state->scope->parent;
}

void parse_declaration_node(ParserState* state, AstNode* node) {
  init_node(node, NODE_DECLARATION);

  node->from = token_start(TOKEN);

  // `test_nodearation` should be guaranteeing a usable identifier here.
  assert(accept(state, TOKEN_IDENTIFIER));
  node->ident = symbol_get(&ACCEPTED.source);

  if (accept_op(state, OP_DECLARE)) {
    node->rhs = parse_type(state);
    node->flags |= (node->rhs->flags & NODE_CONTAINS_ERROR);

  } else if (peek_op(state, OP_DECLARE_ASSIGN)) {
    // We've been invoked from an assignment node, so we're done here.
  } else {
    // We won't be getting here unless we've been called in an unexpected way.
    assert(0);
  }

  node->to = token_end(ACCEPTED);
}

void parse_expression_node(ParserState* state, AstNode* node) {
  init_node(node, NODE_EXPRESSION);

  if (accept(state, TOKEN_LITERAL)) {
    // @TODO Extract this?
    node->flags = EXPR_LITERAL | ACCEPTED.literal_type;
    node->from = token_start(ACCEPTED);
    node->to = token_end(ACCEPTED);
    node->source = ACCEPTED.source;

    if (!ACCEPTED.is_well_formed) {
      node->flags |= NODE_CONTAINS_ERROR;

      if (ACCEPTED.literal_type & IS_STRING_LITERAL) {
        node->error = ERR_UNCLOSED_STRING;
      } else {
        // We don't presently test well-formedness for other literal types.
        assert(0);
      }
    }

  } else if (accept(state, TOKEN_IDENTIFIER)) {
    Symbol name = symbol_get(&ACCEPTED.source);
    FileAddress start = token_start(ACCEPTED);

    if (peek_op(state, OP_OPEN_PAREN)) {
      AstNode* arguments = parse_expression_tuple(state);

      node->flags = EXPR_CALL;
      node->from = start;
      node->to = token_end(ACCEPTED);
      node->ident = name;
      node->rhs = arguments;
      node->scope = state->scope;
      node->flags |= (node->rhs->flags & NODE_CONTAINS_ERROR);
      node->flags |= NODE_CONTAINS_RHS;

    } else {
      node->flags = EXPR_IDENT;
      node->from = start;
      node->to = token_end(ACCEPTED);
      node->ident = name;
      node->scope = state->scope;
    }

  } else if (test_procedure(state)) {
    parse_procedure_node(state, node);

  } else {
    node->from = token_start(TOKEN);
    node->to = token_end(TOKEN);
    node->error = ERR_EXPECTED_EXPRESSION;
    node->flags |= NODE_CONTAINS_ERROR;
  }
}

void parse_assignment_node(ParserState* state, AstNode* node) {
  init_node(node, NODE_ASSIGNMENT);

  // @Lazy Use a better test.
  if (test_declaration(state)) {
    AstNode* decl = parse_declaration(state);
    peek_op(state, OP_ASSIGN) ? accept_op(state, OP_ASSIGN) : accept_op(state, OP_DECLARE_ASSIGN);
    AstNode* value = parse_expression(state);

    node->from = decl->from;
    node->to = token_end(ACCEPTED);
    node->lhs = decl;
    node->rhs = value;
    node->flags |= NODE_CONTAINS_LHS;
    node->flags |= NODE_CONTAINS_RHS;
    node->flags |= (node->lhs->flags & NODE_CONTAINS_ERROR);
    node->flags |= (node->rhs->flags & NODE_CONTAINS_ERROR);

  } else {
    AstNode* expr = parse_expression(state);
    accept_op(state, OP_ASSIGN);
    AstNode* value = parse_expression(state);

    node->from = expr->from;
    node->to = token_end(ACCEPTED);
    node->lhs = expr;
    node->rhs = value;
    node->flags |= NODE_CONTAINS_LHS;
    node->flags |= NODE_CONTAINS_RHS;
    node->flags |= (node->lhs->flags & NODE_CONTAINS_ERROR);
    node->flags |= (node->rhs->flags & NODE_CONTAINS_ERROR);
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

  if (!accept_op(state, OP_NEWLINE)) {
    if (node->flags & NODE_CONTAINS_ERROR) {
      // @TODO More robustly seek past the error.
      while (!peek_op(state, OP_NEWLINE)) state->pos += 1;
    } else {
      AstNode* error = init_node(pool_get(state->nodes), NODE_RECOVERY);
      error->from = token_start(TOKEN);
      error->lhs = node;
      error->error = ERR_EXPECTED_EOL;
      error->flags |= NODE_CONTAINS_LHS;
      error->flags |= NODE_CONTAINS_ERROR;

      // @TODO More robustly seek past the error.
      while (!peek_op(state, OP_NEWLINE)) state->pos += 1;

      error->to = token_end(ACCEPTED);
      return error;
    }
  }

  return node;
}

bool perform_parse_job(ParseJob* job) {
  ParserState* state = new_parser_state(job->tokens);

  while (tokens_remain(state)) {
    if (accept_op(state, OP_NEWLINE)) {
      // Move on, nothing to see here.

    } else {
      AstNode* node = parse_top_level(state);

      if (node->flags & NODE_CONTAINS_ERROR) {
        pipeline_emit_abort_job(job->debug, node);
      } else {
        pipeline_emit_typecheck_job(job->debug, node);
      }

      // print_ast_node_as_tree(state->data.lines, node);
    }
  }

  // print_declaration_list_as_tree(state->data.lines, state->scope->declarations);

  return 1;
}

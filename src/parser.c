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

// ** State Manipulation Primitives ** //

ParserState __parser_state;

#define ACCEPTED       (&__parser_state.list.tokens[__parser_state.pos - 1])
#define TOKEN          (__parser_state.list.tokens[__parser_state.pos])
#define TOKENS_REMAIN  (__parser_state.pos < __parser_state.list.length)
#define CURRENT_LINE   (__parser_state.list.lines[TOKEN.line])
#define CURRENT_SCOPE  (__parser_state.current_scope)

#define ADVANCE()      (__parser_state.pos += 1)
#define BEGIN()        const size_t __mark__ = __parser_state.pos;
#define ROLLBACK()     (__parser_state.pos = __mark__)


// ** Parsing Primitives ** //

int peek(TokenType type) {
  return TOKEN.type == type;
}

int peek_syntax_op(String* op) {
  if (TOKEN.type != TOKEN_SYNTAX_OPERATOR) return 0;
  return string_equals(op, TOKEN.source);
}

int peek_nonsyntax_op(String* op) {
  if (TOKEN.type != TOKEN_OPERATOR) return 0;
  return string_equals(op, TOKEN.source);
}

int peek_op(String* op) {
  return peek_syntax_op(op) || peek_nonsyntax_op(op);
}

int peek_directive(String* name) {
  if (TOKEN.type != TOKEN_DIRECTIVE) return 0;
  return string_equals(name, TOKEN.source);
}

int accept(TokenType type) {
  if (!peek(type)) return 0;

  ADVANCE();
  return 1;
}

int accept_op(String* op) {
  if (!peek_op(op)) return 0;

  ADVANCE();
  return 1;
}

int accept_syntax_op(String* op) {
  if (!peek_syntax_op(op)) return 0;

  ADVANCE();
  return 1;
}

int accept_nonsyntax_op(String* op) {
  if (!peek_nonsyntax_op(op)) return 0;

  ADVANCE();
  return 1;
}

int accept_directive(String* name) {
  if (!peek_directive(name)) return 0;

  ADVANCE();
  return 1;
}

void error(char* msg) {
  char* filename = to_zero_terminated_string(TOKEN.file);
  char* line_str = to_zero_terminated_string(&CURRENT_LINE);
  size_t line_no = TOKEN.line;

  char* bold = "\e[1;37m";
  char* code = "\e[0;36m";
  char* err = "\e[0;31m";
  char* reset = "\e[0m";

  printf("Error: %s\n", msg);
  printf("In %s%s%s on line %s%zu%s\n\n", bold, filename, reset, bold, TOKEN.line + 1, reset);
  printf("> %s%s%s\n", code, line_str, reset);

  for (int i = 0; i < CURRENT_LINE.length; i++) line_str[i] = ' ';
  for (int i = TOKEN.pos; i < TOKEN.pos + TOKEN.source->length; i++) line_str[i] = '^';

  printf("  %s%s%s\n\n", err, line_str, reset);

  free(filename);
  free(line_str);
}


// ** Apathetic Parsing ** //

// @Lazy We should be able to parse this fully in the initial pass, since no
//       unknown operators are legal in the return types list.
// @Lazy Is there any reason a '{' should be permitted in a return type block?
List* slurp_return_list() {
  List* tokens = new_list(1, 8);  // @TODO Validate these numbers.

  while(!peek_op(OP_OPEN_BRACE)) {
    list_add(tokens, &TOKEN);
    ADVANCE();
  }

  return tokens;
}

List* slurp_code_block() {
  size_t parens = 0;
  size_t braces = 0;
  List* tokens = new_list(1, 8);  // @TODO Validate these numbers.

  while (!(TOKENS_REMAIN && parens == 0 && braces == 0 && peek_op(OP_CLOSE_BRACE))) {
    if (peek_op(OP_OPEN_PAREN)) parens += 1;
    if (peek_op(OP_CLOSE_PAREN)) parens -= 1;
    if (peek_op(OP_OPEN_BRACE)) braces += 1;
    if (peek_op(OP_CLOSE_BRACE)) braces -= 1;

    list_add(tokens, &TOKEN);
    ADVANCE();
  }

  return tokens;
}

// ** Lookahead Operations ** //

int test_declaration() {
  int result = 0;

  BEGIN();
  if (accept(TOKEN_IDENTIFIER)) {
    result = peek_op(OP_DECLARE) || peek_op(OP_DECLARE_ASSIGN);
  }
  ROLLBACK();

  return result;
}

int test_directive() {
  return peek(TOKEN_DIRECTIVE);
}

int test_function_expression() {
  int result = 0;

  BEGIN();
  if (accept_op(OP_OPEN_PAREN)) {
    size_t depth = 0;

    while (TOKENS_REMAIN && (depth > 0 || !peek_op(OP_CLOSE_PAREN))) {
      if (peek_op(OP_OPEN_PAREN)) depth += 1;
      if (peek_op(OP_CLOSE_PAREN)) depth -= 1;
      ADVANCE();
    }

    if (accept_op(OP_CLOSE_PAREN)) {
      result = peek_op(OP_FUNC_ARROW);
    }
  }
  ROLLBACK();

  return result;
}


// ** Parser States ** //
void* parse_declaration();  // @TODO Make this forward declaration unnecessary.

void* parse_argument_list() {
  List* arguments = new_list(1, 8);

  if (peek_op(OP_CLOSE_PAREN)) return arguments;

  Declaration* decl = parse_declaration();
  if (decl == NULL) {
    error("Got a NULL declaration when we shouldn't have");
    return arguments;
  }
  list_add(arguments, decl);

  while (accept_op(OP_COMMA)) {
    Declaration* decl = parse_declaration();
    if (decl == NULL) {
      error("Got a NULL declaration when we shouldn't have");
      return arguments;
    }
    list_add(arguments, decl);
  }

  return arguments;
}

void* parse_return_type() {
  Token* name = NULL;
  Token* type = NULL;

  if (accept(TOKEN_IDENTIFIER)) {
    type = ACCEPTED;

    if (accept_op(OP_DECLARE)) {
      if (accept(TOKEN_IDENTIFIER)) {
        name = type;
        type = ACCEPTED;
      } else {
        error("Expected type in return type declaration");
        return NULL;
      }
    } else {
      // Type-only return type.
    }
  } else {
    error("Expected type in return type declaration");
    return NULL;
  }

  ReturnType* return_type = malloc(sizeof(ReturnType));
  return_type->type = type;
  return_type->name = name;

  return return_type;
}

void* parse_return_list() {
  List* returns = new_list(1, 8);

  if (accept_op(OP_OPEN_PAREN)) {
    ReturnType* type = parse_return_type();
    if (type == NULL) return returns;

    list_add(returns, type);

    while (accept_op(OP_COMMA)) {
      ReturnType* type = parse_return_type();
      if (type == NULL) return returns;
      list_add(returns, type);
    }

    if (!accept_op(OP_CLOSE_PAREN)) {
      error("Unbalanced parens in return type list");
      return returns;
    }
  } else if (accept(TOKEN_IDENTIFIER)) {
    ReturnType* type = malloc(sizeof(ReturnType));
    type->type = ACCEPTED;
    type->name = NULL;
    list_add(returns, type);
  } else {
    // Void return
  }

  return returns;
}

void* parse_function_expression() {
  accept_op(OP_OPEN_PAREN);
  List* args = parse_argument_list();
  accept_op(OP_CLOSE_PAREN);

  accept_op(OP_FUNC_ARROW);

  List* returns = parse_return_list();

  if (!accept_op(OP_OPEN_BRACE)) {
    // @Leak args, returns
    error("Expected code block");
    return NULL;
  }
  List* body = slurp_code_block();
  if (!accept_op(OP_CLOSE_BRACE)) {
    // @Leak args, returns, body
    error("Unexpected termination of code block");
    return NULL;
  }

  FunctionExpression* expr = calloc(1, sizeof(FunctionExpression));
  expr->base.type = EXPR_FUNCTION;
  expr->arguments = args;
  expr->returns = returns;
  expr->body = body;

  return expr;
}

// @Cleanup Replace these dynamic allocations with a growable pool.
void* parse_expression() {
  Expression* result = NULL;

  if (accept(TOKEN_IDENTIFIER)) {
    IdentifierExpression* expr = malloc(sizeof(IdentifierExpression));
    expr->base.type = EXPR_IDENT;
    expr->identifier = ACCEPTED;

    result = (Expression*) expr;
  } else if (accept(TOKEN_NUMBER_DECIMAL) ||
             accept(TOKEN_NUMBER_FRACTIONAL) ||
             accept(TOKEN_NUMBER_HEX) ||
             accept(TOKEN_NUMBER_BINARY)) {
    LiteralExpression* expr = malloc(sizeof(LiteralExpression));
    expr->base.type = EXPR_LITERAL;
    expr->literal = ACCEPTED;

    result = (Expression*) expr;
  } else if (accept(TOKEN_STRING)) {
    // @TODO Push well-formedness checks into the tokenizer.
    if (ACCEPTED->source->data[0] != ACCEPTED->source->data[ACCEPTED->source->length - 1]) {
      error("Unclosed string literal.");
      return NULL;
    }

    LiteralExpression* expr = malloc(sizeof(LiteralExpression));
    expr->base.type = EXPR_LITERAL;
    expr->literal = ACCEPTED;

    result = (Expression*) expr;
  } else if (test_function_expression()) {
    result = parse_function_expression();
  } else if (accept_op(OP_OPEN_PAREN)) {
    UnaryOpExpression* expr = malloc(sizeof(UnaryOpExpression));
    expr->base.type = EXPR_UNARY_OP;
    expr->operator = ACCEPTED;
    expr->rhs = parse_expression();

    if (!accept_op(OP_CLOSE_PAREN)) {
      // @Leak expr
      error("Expected ')'");
      return NULL;
    }

    result = (Expression*) expr;
  } else if (accept(TOKEN_OPERATOR)) {
    UnaryOpExpression* expr = malloc(sizeof(UnaryOpExpression));
    expr->base.type = EXPR_UNARY_OP;
    expr->operator = ACCEPTED;
    expr->rhs = parse_expression();

    if (expr->rhs == NULL) {
      return NULL;
    }

    // @Lazy I know there's a cleaner way to handle this.
    {
      UnaryOpExpression* parent = expr;
      Expression* subexpr = expr->rhs;
      while (((UnaryOpExpression*) parent)->rhs->type == EXPR_BINARY_OP) {
        parent = (void*) subexpr;
        subexpr = parent->rhs;
      }

      if (parent != expr) {
        result = expr->rhs;
        parent->rhs = (Expression*) expr;
        expr->rhs = subexpr;
      } else {
        result = (Expression*) expr;
      }
    }

    return result;
  } else {
    error("Unable to parse expression");
    return NULL;
  }

  if (accept(TOKEN_OPERATOR)) {
    BinaryOpExpression* expr = malloc(sizeof(BinaryOpExpression));
    expr->base.type = EXPR_BINARY_OP;
    expr->operator = ACCEPTED;
    expr->lhs = result;
    expr->rhs = parse_expression();

    if (expr->rhs == NULL) {
      // @Leak expr
      return NULL;
    }

    result = (Expression*) expr;
  }

  return result;
}

void* parse_type() {
  if (!accept(TOKEN_IDENTIFIER)) {
    error("Unknown expression; expected a type identifier");
    return NULL;
  }

  return ACCEPTED;
}

void* parse_directive() {
  // @TODO Actually process this directive.
  accept(TOKEN_DIRECTIVE);
  return NULL;
}

void* parse_declaration() {
  Token* name = NULL;
  Token* type = NULL;
  Expression* value = NULL;

  accept(TOKEN_IDENTIFIER);
  name = ACCEPTED;

  if (accept_op(OP_DECLARE)) {
    type = parse_type();
    if (type == NULL) {
      return NULL;
    }

    if (accept_op(OP_ASSIGN)) {
      value = parse_expression();
    }
  } else if (accept_op(OP_DECLARE_ASSIGN)) {
    value = parse_expression();
    if (value == NULL) {
      return NULL;
    }
  }

  // @Lazy Is this the best place to handle this?
  if (value != NULL && value->type == EXPR_FUNCTION) {
    ((FunctionExpression*) value)->name = name->source;
  }

  Declaration* decl = malloc(sizeof(Declaration));
  decl->name = name;
  decl->type = type;
  decl->value = value;

  return decl;
}

void parse_namespace() {
  while (TOKENS_REMAIN) {
    if (accept(TOKEN_NEWLINE)) {
      // Move on, nothing to see here.
    } else if (test_directive()) {
      void* directive = parse_directive();

      // @TODO Actually implement these directives.
      error("Hey, got a directive");
    } else if (test_declaration()) {
      Declaration* decl = parse_declaration();
      if (decl == NULL) {
        error("Malformed declaration");
        while (accept(TOKEN_NEWLINE));
        continue;
      }

      if (!accept(TOKEN_NEWLINE)) {
        error("Expected end of declaration");
        continue;
      }

      // @TODO Figure this out better.
      ParserScope* scope = CURRENT_SCOPE;
      list_add(scope->declarations, decl);
    } else {
      error("Unrecognized code in top-level context");
      while (accept(TOKEN_NEWLINE));
    }
  }
}

void parse_file(TokenList* list, ParserScope* global) {
  __parser_state = (ParserState) {
    .list = *list,
    .pos = 0,
    .current_scope = global,
  };

  parse_namespace();
}

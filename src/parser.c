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
// #define NEXT_TOKEN     (__parser_state.tokens[__parser_state.pos + 1])
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

int peek_op(String* op) {
  if (TOKEN.type != TOKEN_OPERATOR) return 0;
  return string_equals(op, TOKEN.source);
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

List* slurp_argument_list() {
  size_t depth = 0;
  List* tokens = new_list(2, 64);  // @TODO Validate these numbers.

  while (!(TOKENS_REMAIN && depth == 0 && peek_op(OP_CLOSE_PAREN))) {
    if (peek_op(OP_OPEN_PAREN)) depth += 1;
    if (peek_op(OP_CLOSE_PAREN)) depth -= 1;

    list_add(tokens, &TOKEN);
    ADVANCE();
  }

  return tokens;
}

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
  int result;

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
    slurp_argument_list();  // @Leak We really don't need an allocation here.
    if (accept_op(OP_CLOSE_PAREN)) {
      result = peek_op(OP_FUNC_ARROW);
    }
  }
  ROLLBACK();

  return result;
}


// ** Parser States ** //

void* parse_function_expression() {
  accept_op(OP_OPEN_PAREN);
  List* args = slurp_argument_list();
  accept_op(OP_CLOSE_PAREN);

  accept_op(OP_FUNC_ARROW);

  List* returns = slurp_return_list();

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

  FunctionExpression* expr = malloc(sizeof(FunctionExpression));
  expr->base.type = EXPR_UNPARSED_FUNCTION;
  expr->arguments = args;
  expr->returns = returns;
  expr->body = body;

  return expr;
}

// @Cleanup Replace these dynamic allocations with a growable pool.
void* parse_expression() {
  if (accept(TOKEN_IDENTIFIER)) {
    IdentifierExpression* expr = malloc(sizeof(IdentifierExpression));
    expr->base.type = EXPR_IDENT;
    expr->identifier = ACCEPTED;

    return expr;
  } else if (accept(TOKEN_NUMBER_DECIMAL) ||
             accept(TOKEN_NUMBER_FRACTIONAL) ||
             accept(TOKEN_NUMBER_HEX) ||
             accept(TOKEN_NUMBER_BINARY)) {
    LiteralExpression* expr = malloc(sizeof(LiteralExpression));
    expr->base.type = EXPR_LITERAL;
    expr->literal = ACCEPTED;

    return expr;
  } else if (accept(TOKEN_STRING)) {
    // @TODO Push well-formedness checks into the tokenizer.
    if (ACCEPTED->source->data[0] == ACCEPTED->source->data[ACCEPTED->source->length - 1]) {
      LiteralExpression* expr = malloc(sizeof(LiteralExpression));
      expr->base.type = EXPR_LITERAL;
      expr->literal = ACCEPTED;

      return expr;
    } else {
      error("Unclosed string literal.");
      return NULL;
    }
  } else if (test_function_expression()) {
      return parse_function_expression();
  } else if (accept_op(OP_OPEN_PAREN)) {
    Expression* expr = parse_expression();
    if (!accept_op(OP_CLOSE_PAREN)) {
      // @Leak expr
      error("Expected ')'");
      return NULL;
    }

    return expr;
  } else {
    error("Unable to parse expression");
  }

  return NULL;
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

  Declaration* decl = malloc(sizeof(Declaration));
  decl->name = name;
  decl->type = type;
  decl->value = value;

  return decl;
}

void parse_namespace() {
  while (TOKENS_REMAIN) {
    if (test_directive()) {
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

      while (accept(TOKEN_NEWLINE));

      if (decl != NULL) {
        // @TODO Figure this out.
        ParserScope* scope = CURRENT_SCOPE;
        String* key = decl->name->source;

        List* declarations = table_find(scope->declarations, key);
        if (declarations == NULL) {
          declarations = new_list(1, 128);
          table_add(scope->declarations, key, declarations);
        }

        list_add(declarations, decl);
      }
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

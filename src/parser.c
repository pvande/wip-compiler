/*********

PROGRAM     => STATEMENT*
STATEMENT   => _ (DECLARATION) _ (Newline | <EOF>)
DECLARATION => Identifier _ ":" _ TYPE |
               Identifier _ ":" _ TYPE _ "=" _ EXPRESSION |
               Identifier _ ":=" _ EXPRESSION
TYPE        => Identifier
EXPRESSION  => Identifier |
               Number |
               String
_ => Whitespace?

*********/

/*
  PROGRAM => STATEMENT*
  STATEMENT => _? (DECLARATION) _? (NEWLINE | EOF)
  DECLARATION => IDENTIFIER _? ':' (_? IDENTIFIER _?)? '=' _? EXPRESSION
  EXPRESSION => UNARY _? EXPRESSION | EXPRESSION _? BINARY _? EXPRESSION | IDENTIFIER '(' _? (EXPRESSION _? (',' _? EXPRESSION _?)*)? ')'| VALUE
  UNARY => ()
  BINARY => ()
  VALUE => (NUMBER | STRING | IDENTIFIER)
*/

ParserScope* new_parser_scope() {
  ParserScope* scope = calloc(1, sizeof(ParserScope));
  scope->declarations = new_table(128);

  return scope;
}

ParserState __parser_state;

#define ACCEPTED       (__parser_state.tokens[__parser_state.pos - 1])
#define TOKEN          (__parser_state.tokens[__parser_state.pos])
#define TOKENS_REMAIN  (__parser_state.pos < __parser_state.length)
#define CURRENT_LINE   (__parser_state.lines[TOKEN.line])
#define CURRENT_SCOPE  (__parser_state.current_scope)

#define ADVANCE()      (__parser_state.pos += 1)
#define RETREAT()      (__parser_state.pos -= 1)

Expression* new_identifier_expression(Token* token) {
  ExpressionIdentifier* expr = malloc(sizeof(ExpressionIdentifier));
  expr->identifier = token;

  return (Expression*) expr;
}

Expression* new_literal_expression(Token* token) {
  ExpressionLiteral* expr = malloc(sizeof(ExpressionLiteral));
  expr->literal = token;

  return (Expression*) expr;
}

Declaration* new_declaration(Token* ident, Token* type, Expression* value) {
  Declaration* decl = malloc(sizeof(Declaration));
  decl->name = ident;
  decl->type = type;
  decl->value = value;

  return decl;
}

void add_declaration(Declaration* decl) {
  ParserScope* scope = CURRENT_SCOPE;
  String* key = decl->name->source;

  List* declarations = table_find(scope->declarations, key);
  if (declarations == NULL) {
    declarations = new_list(1, 128);
    table_add(scope->declarations, key, declarations);
  }

  list_add(declarations, decl);
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
}

int accept(TokenType type) {
  if (TOKEN.type == type) {
    ADVANCE();
    return 1;
  }

  return 0;
}

int accept_op(char* op) {
  String* oper = new_string(op);

  accept(TOKEN_WHITESPACE);
  if (TOKEN.type == TOKEN_OPERATOR && string_equals(oper, TOKEN.source)) {
    ADVANCE();
    return 1;
  }

  return 0;
}

Expression* parse_expression() {
  if (accept(TOKEN_IDENTIFIER)) {
    return new_identifier_expression(&ACCEPTED);
  } else if (accept(TOKEN_NUMBER_DECIMAL)) {
    return new_literal_expression(&ACCEPTED);
  } else if (accept(TOKEN_NUMBER_FRACTIONAL)) {
    return new_literal_expression(&ACCEPTED);
  } else if (accept(TOKEN_NUMBER_HEX)) {
    return new_literal_expression(&ACCEPTED);
  } else if (accept(TOKEN_NUMBER_BINARY)) {
    return new_literal_expression(&ACCEPTED);
  } else if (accept(TOKEN_STRING)) {
    if (ACCEPTED.source->data[0] != ACCEPTED.source->data[ACCEPTED.source->length - 1]) {
      RETREAT();
      error("Unclosed string literal.");
      return NULL;
    }
    return new_literal_expression(&ACCEPTED);
  } else {
    error("Unable to parse expression.");
  }

  return NULL;
}

Declaration* parse_declaration() {
  Declaration* decl;

  if (accept(TOKEN_IDENTIFIER)) {
    Token* ident = &ACCEPTED;

    if (accept_op(":")) {
      // @TODO Parse type expressions
      accept(TOKEN_WHITESPACE);
      if (!accept(TOKEN_IDENTIFIER)) {
        error("Expected type expression.");
        return NULL;
      }
      Token* type = &ACCEPTED;
      decl = new_declaration(ident, type, NULL);

      if (accept_op("=")) {
        accept(TOKEN_WHITESPACE);
        Expression* value = parse_expression();
        if (!value) {
          error("Expected expression.");
          return NULL;
        }

        decl->value = value;
      }
    } else if (accept_op(":=")) {
      accept(TOKEN_WHITESPACE);
      Expression* value = parse_expression();
      if (!value) {
        error("Expected expression.");
        return NULL;
      }

      decl = new_declaration(ident, NULL, value);
    } else {
      error("Expected a declaration.");
      return NULL;
    }
  } else if (accept(TOKEN_DIRECTIVE)) {
    // @TODO Add support for directives at the declaration level.
  } else {
    error("Expected an identifier to declare.");
    return NULL;
  }

  accept(TOKEN_WHITESPACE);
  if (!accept(TOKEN_NEWLINE)) {
    error("Expected end of line.");
    return NULL;
  }

  return decl;
}

void parse_tokens(TokenList* list, ParserScope* scope) {
  __parser_state = (ParserState) {
    .tokens = list->tokens,
    .lines = list->lines,
    .length = list->length,

    .pos = 0,

    .current_scope = scope,
  };

  while (TOKENS_REMAIN) {
    accept(TOKEN_WHITESPACE);
    Declaration* decl = parse_declaration();
    if (decl != NULL) {
      add_declaration(decl);
    } else {
      break;
    }
  }
}

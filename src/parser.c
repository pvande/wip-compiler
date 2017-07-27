/*
  PROGRAM => STATEMENT*
  STATEMENT => _? (DECLARATION) _? (NEWLINE | EOF)
  DECLARATION => IDENTIFIER _? ':' (_? IDENTIFIER _?)? '=' _? EXPRESSION
  EXPRESSION => UNARY _? EXPRESSION | EXPRESSION _? BINARY _? EXPRESSION | IDENTIFIER '(' _? (EXPRESSION _? (',' _? EXPRESSION _?)*)? ')'| VALUE
  UNARY => ()
  BINARY => ()
  VALUE => (NUMBER | STRING | IDENTIFIER)
  _ => (' ' | '\t')+
  NEWLINE => '\n'+
  EOF => '\0'
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

Declaration* add_declaration(Token* ident, Token* type, Token* value) {
  ParserScope* scope = CURRENT_SCOPE;

  List* declarations = table_find(scope->declarations, ident->source);
  if (declarations == NULL) {
    declarations = new_list(1, 128);
    table_add(scope->declarations, ident->source, declarations);
  }

  Declaration* decl = malloc(sizeof(Declaration));
  decl->name = ident;
  decl->type = type;
  decl->value = value;

  list_add(declarations, decl);

  return decl;
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

int expect(TokenType type) {
  if (TOKEN.type == type) {
    ADVANCE();
    return 1;
  }

  error("Expected different token.");
  while(TOKEN.type != TOKEN_NEWLINE) ADVANCE();

  return 0;
}

void parse_statement() {
  if (accept(TOKEN_IDENTIFIER)) {
    Token* ident = &ACCEPTED;

    if (accept_op(":")) {
      // @TODO Parse type expressions
      accept(TOKEN_WHITESPACE);
      expect(TOKEN_IDENTIFIER);
      Token* type = &ACCEPTED;
      Token* value = NULL;

      if (accept_op("=")) {
        // @TODO Parse expressions
        accept(TOKEN_WHITESPACE);
        expect(TOKEN_IDENTIFIER);
        value = &ACCEPTED;
      }

      add_declaration(ident, type, value);
    } else if (accept_op(":=")) {
      accept(TOKEN_WHITESPACE);
      expect(TOKEN_IDENTIFIER);
      Token* value = &ACCEPTED;

      add_declaration(ident, NULL, value);
    }
  }

  expect(TOKEN_NEWLINE);
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
    parse_statement();
  }
}

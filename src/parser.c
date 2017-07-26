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

typedef struct {
  int id;
} Identifier;

typedef struct {
} Expression;

typedef struct {
  Identifier* identifier;
} Declaration;

typedef struct ParserScope {
  Table* declarations;

  struct ParserScope* parent_scope;
} ParserScope;

typedef struct {
  Token* tokens;
  String* lines;
  size_t length;
  size_t pos;

  ParserScope* global;
  ParserScope* current_scope;
} ParserState;

ParserScope* new_parser_scope() {
  ParserScope* scope = malloc(sizeof(ParserScope));
  // scope->identifiers = new_table(128);
  scope->declarations = new_table(128);

  return scope;
}


#define THIS                  (state->tokens[state->pos])
#define NEXT                  (state->tokens[state->pos + 1])
#define THIS_IS(TYPE)         (THIS.type == TYPE)
#define NEXT_IS(TYPE)         (NEXT.type == TYPE)
#define NEXT_IS_OP(STR)       (NEXT_IS(TOKEN_TYPE_OPERATOR) && string_equals(NEXT.source, new_string(STR)))
#define TOKENS_REMAIN         (state->pos < state->length)
#define ADVANCE()             if (TOKENS_REMAIN) { state->pos += 1; }
#define SKIP_REST_OF_LINE()   while (!THIS_IS(TOKEN_TYPE_NEWLINE)) { ADVANCE(); }

#define CONSUME_WHITESPACE()  if (THIS_IS(TOKEN_TYPE_WHITESPACE)) { ADVANCE(); }

#define PRINT_ERROR_MSG(MSG)    printf("Error: %s\n", MSG);
#define PRINT_ERROR_LOC(TOKEN)  printf("In \e[1;37m%s\e[0m on line \e[1;37m%zu\e[0m\n", to_zero_terminated_string(TOKEN.file), TOKEN.line + 1);
#define PRINT_ERROR_LINE(TOKEN)  printf("> \e[0;36m%s\e[0m\n", to_zero_terminated_string(&state->lines[TOKEN.line]));
#define PRINT_ERROR_INDICATOR(TOKEN)  do { for (int i = 0;  i < TOKEN.pos + 2; i++, putchar(' ')); for (int i = 0; i < TOKEN.source->length; i++, putchar('^')); printf("\n\n"); } while (0)

#define ERROR(MSG, TOKEN)  do { PRINT_ERROR_MSG(MSG); PRINT_ERROR_LOC(TOKEN); PRINT_ERROR_LINE(TOKEN); PRINT_ERROR_INDICATOR(TOKEN); SKIP_REST_OF_LINE(); } while (0)

// void add_declaration(ParserState state, String* name) {
//   ParserScope* scope = state.current_scope;
//
//   List* declarations = table_get(scope->declarations, name);
// }

void parse_statement(ParserState* state) {
  CONSUME_WHITESPACE();

  if (0 && THIS_IS(TOKEN_TYPE_IDENTIFIER) && NEXT_IS_OP(":")) {
    // Declaration d = state.current_scope->declarations[state.current_scope->declaration_count];
  } else {
    ERROR("not handled", THIS);
  }

  assert(THIS_IS(TOKEN_TYPE_NEWLINE));
  ADVANCE();
}

void parse_tokens(TokenList* list, ParserScope* scope) {
  ParserState* state = malloc(sizeof(ParserState));
  state->tokens = list->tokens;
  state->lines = list->lines;
  state->length = list->length;
  state->global = scope;
  state->current_scope = scope;

  while (TOKENS_REMAIN) {
    parse_statement(state);
  }
}

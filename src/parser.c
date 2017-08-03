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

// ** State Manipulation Primitives ** //

bool tokens_remain(ParserState* state) {
  return state->pos < state->data.length;
}

// ParserState __parser_state;
//
#define ACCEPTED       (state->data.tokens[state->pos - 1])
#define TOKEN          (state->data.tokens[state->pos])
// #define TOKENS_REMAIN  (__parser_state.pos < __parser_state.list.length)
// #define CURRENT_LINE   (__parser_state.list.lines[TOKEN.line])
// #define CURRENT_SCOPE  (__parser_state.current_scope)
//
// #define ADVANCE()      (__parser_state.pos += 1)
// #define BEGIN()        const size_t __mark__ = __parser_state.pos;
// #define ROLLBACK()     (__parser_state.pos = __mark__)


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

// int peek_directive(String* name) {
//   if (TOKEN.type != TOKEN_DIRECTIVE) return 0;
//   return string_equals(name, &TOKEN.source);
// }

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

// int accept_directive(String* name) {
//   if (!peek_directive(name)) return 0;
//
//   ADVANCE();
//   return 1;
// }
//
// void error(char* msg) {
//   char* filename = to_zero_terminated_string(&TOKEN.file);
//   char* line_str = to_zero_terminated_string(&CURRENT_LINE);
//   size_t line_no = TOKEN.line;
//
//   char* bold = "\e[1;37m";
//   char* code = "\e[0;36m";
//   char* err = "\e[0;31m";
//   char* reset = "\e[0m";
//
//   printf("Error: %s\n", msg);
//   printf("In %s%s%s on line %s%zu%s\n\n", bold, filename, reset, bold, TOKEN.line + 1, reset);
//   printf("> %s%s%s\n", code, line_str, reset);
//
//   for (int i = 0; i < CURRENT_LINE.length; i++) line_str[i] = ' ';
//   for (int i = TOKEN.pos; i < TOKEN.pos + TOKEN.source.length; i++) line_str[i] = '^';
//
//   printf("  %s%s%s\n\n", err, line_str, reset);
//
//   free(filename);
//   free(line_str);
// }
//
//
// // ** Apathetic Parsing ** //
//
// // @Lazy We should be able to parse this fully in the initial pass, since no
// //       unknown operators are legal in the return types list.
// // @Lazy Is there any reason a '{' should be permitted in a return type block?
// List* slurp_return_list() {
//   List* tokens = new_list(1, 8);  // @TODO Validate these numbers.
//
//   while(!peek_op(OP_OPEN_BRACE)) {
//     list_append(tokens, &TOKEN);
//     ADVANCE();
//   }
//
//   return tokens;
// }
//
// List* slurp_code_block() {
//   size_t parens = 0;
//   size_t braces = 0;
//   List* tokens = new_list(1, 8);  // @TODO Validate these numbers.
//
//   while (!(TOKENS_REMAIN && parens == 0 && braces == 0 && peek_op(OP_CLOSE_BRACE))) {
//     if (peek_op(OP_OPEN_PAREN)) parens += 1;
//     if (peek_op(OP_CLOSE_PAREN)) parens -= 1;
//     if (peek_op(OP_OPEN_BRACE)) braces += 1;
//     if (peek_op(OP_CLOSE_BRACE)) braces -= 1;
//
//     list_append(tokens, &TOKEN);
//     ADVANCE();
//   }
//
//   return tokens;
// }

// ** Lookahead Operations ** //

int test_declaration(ParserState* state) {
  int result = 0;

  size_t mark = state->pos;
  if (accept(state, TOKEN_IDENTIFIER)) {
    result = peek_op(state, OP_DECLARE);
  }
  state->pos = mark;

  return result;
}

// int test_directive() {
//   return peek(TOKEN_DIRECTIVE);
// }
//
// int test_code_block() {
//   return peek_op(OP_OPEN_BRACE);
// }
//
// int test_function_expression() {
//   int result = 0;
//
//   BEGIN();
//   if (accept_op(OP_OPEN_PAREN)) {
//     size_t depth = 0;
//
//     while (TOKENS_REMAIN && (depth > 0 || !peek_op(OP_CLOSE_PAREN))) {
//       if (peek_op(OP_OPEN_PAREN)) depth += 1;
//       if (peek_op(OP_CLOSE_PAREN)) depth -= 1;
//       ADVANCE();
//     }
//
//     if (accept_op(OP_CLOSE_PAREN)) {
//       result = peek_op(OP_FUNC_ARROW);
//     }
//   }
//   ROLLBACK();
//
//   return result;
// }


// ** Parser States ** //

// TYPE = Identifier
void parse_type(ParserState* state, AstNode* type) {
  type->from = (FileAddress) { TOKEN.line, TOKEN.pos };
  type->to   = (FileAddress) { TOKEN.line, TOKEN.pos + TOKEN.source.length };

  if (accept(state, TOKEN_IDENTIFIER)) {
    type->ident = symbol_get(&ACCEPTED.source);
  } else {
    type->error = new_string("Expected a type identifier");
  }
}

// DECLARATION = Identifier ":" TYPE
//             | Identifier ":" TYPE "=" EXPRESSION    @TODO
//             | Identifier ":=" EXPRESSION            @TODO
void parse_declaration(ParserState* state, AstNode* decl) {
  assert(accept(state, TOKEN_IDENTIFIER));
  decl->ident = symbol_get(&ACCEPTED.source);
  decl->from = (FileAddress) { ACCEPTED.line, ACCEPTED.pos };

  assert(accept_op(state, OP_DECLARE));

  decl->lhs = pool_get(state->nodes);
  parse_type(state, decl->lhs);

  decl->to = (FileAddress) { ACCEPTED.line, ACCEPTED.pos + ACCEPTED.source.length };
}

// void* parse_declaration_tuple() {
//   if (!accept_op(OP_OPEN_PAREN)) {
//     error("Expected argument list");
//     return NULL;
//   }
//
//   List* declarations = new_list(1, 8);
//
//   if (accept_op(OP_CLOSE_PAREN)) return declarations;
//
//   do {
//     AstDeclaration* decl = parse_declaration();
//     if (decl == NULL) {
//       error("Got a NULL declaration when we shouldn't have");
//       return declarations;
//     }
//     list_append(declarations, decl);
//   } while (accept_op(OP_COMMA));
//
//   if (!accept_op(OP_CLOSE_PAREN)) {
//     error("Expected to find the end of the argument list");
//     return declarations;
//   }
//
//   return declarations;
// }
//
// void* parse_expression_tuple() {
//   if (!accept_op(OP_OPEN_PAREN)) {
//     error("Expected argument list");
//     return NULL;
//   }
//
//   List* expressions = new_list(1, 8);
//
//   if (accept_op(OP_CLOSE_PAREN)) return expressions;
//
//   do {
//     AstDeclaration* decl = parse_expression();
//     if (decl == NULL) {
//       error("Got a NULL expression when we shouldn't have");
//       return expressions;
//     }
//     list_append(expressions, decl);
//   } while (accept_op(OP_COMMA));
//
//   if (!accept_op(OP_CLOSE_PAREN)) {
//     error("Expected to find the end of the argument list");
//     return expressions;
//   }
//
//   return expressions;
// }
//
// void* parse_return_type() {
//   // @TODO Handle named return values.
//   // @TODO Handle multiple return values.
//
//   if (peek(TOKEN_IDENTIFIER)) {
//     return parse_type();
//   } else if (peek_op(OP_OPEN_BRACE)) {
//     AstType* type = malloc(sizeof(AstType));
//     String* name = new_string("void");  // @TODO Really?  We're allocating here?
//     type->name = symbol_get(name);
//     free(name);
//     return type;
//   } else {
//     error("Expected return type, got whatever this is...");
//     return NULL;
//   }
// }
//
// void* parse_code_block() {
//   if (!accept_op(OP_OPEN_BRACE)) {
//     error("Expected code block");
//     return NULL;
//   }
//
//   List* body = new_list(4, 8);
//
//   while (!peek_op(OP_CLOSE_BRACE)) {
//     accept(TOKEN_NEWLINE);
//     if (test_declaration()) {
//       AstDeclaration* decl = parse_declaration();
//       AstStatement* stmt = calloc(1, sizeof(AstStatement));
//       stmt->type = STATEMENT_DECLARATION;
//       stmt->data = decl;
//
//       list_append(body, stmt);
//     } else {
//       AstExpression* expr = parse_expression();
//       AstStatement* stmt = calloc(1, sizeof(AstStatement));
//       stmt->type = STATEMENT_EXPRESSION;
//       stmt->data = expr;
//
//       list_append(body, stmt);
//     }
//     accept(TOKEN_NEWLINE);
//   }
//
//   // List* body = slurp_code_block();
//   if (!accept_op(OP_CLOSE_BRACE)) {
//     // @Leak args, returns, body
//     error("Unexpected termination of code block");
//     return NULL;
//   }
//
//   return body;
// }
//
// void* parse_function_expression() {
//   List* args = parse_declaration_tuple();
//
//   accept_op(OP_FUNC_ARROW);
//
//   AstType* returns = parse_return_type();
//
//   List* body = parse_code_block();
//
//   FunctionExpression* expr = calloc(1, sizeof(FunctionExpression));
//   expr->base.type = EXPR_FUNCTION;
//   expr->arguments = args;
//   expr->returns = *returns;
//   expr->body = body;
//
//   free(returns);
//
//   return expr;
// }
//
// // @Cleanup Replace these dynamic allocations with a growable pool.
// void* parse_expression() {
//   AstExpression* result = NULL;
//
//   if (accept(TOKEN_IDENTIFIER) && peek_syntax_op(OP_OPEN_PAREN)) {
//     CallExpression* expr = malloc(sizeof(CallExpression));
//     expr->base.type = EXPR_CALL;
//     expr->function = symbol_get(&ACCEPTED->source);
//     expr->arguments = parse_expression_tuple();
//
//     result = (AstExpression*) expr;
//   } else if (accept(TOKEN_IDENTIFIER)) {
//     IdentifierExpression* expr = malloc(sizeof(IdentifierExpression));
//     expr->base.type = EXPR_IDENT;
//     expr->identifier = ACCEPTED;
//
//     result = (AstExpression*) expr;
//   } else if (accept(TOKEN_LITERAL)) {
//     if (! ACCEPTED->is_well_formed) {
//       error("Malformed literal.");
//       return NULL;
//     }
//
//     LiteralExpression* expr = malloc(sizeof(LiteralExpression));
//     expr->base.type = EXPR_LITERAL;
//     expr->literal = ACCEPTED;
//
//     result = (AstExpression*) expr;
//   } else if (test_function_expression()) {
//     result = parse_function_expression();
//   } else if (accept_op(OP_OPEN_PAREN)) {
//     UnaryOpExpression* expr = malloc(sizeof(UnaryOpExpression));
//     expr->base.type = EXPR_UNARY_OP;
//     expr->operator = ACCEPTED;
//     expr->rhs = parse_expression();
//
//     if (!accept_op(OP_CLOSE_PAREN)) {
//       // @Leak expr
//       error("Expected ')'");
//       return NULL;
//     }
//
//     result = (AstExpression*) expr;
//   } else if (accept(TOKEN_OPERATOR)) {
//     UnaryOpExpression* expr = malloc(sizeof(UnaryOpExpression));
//     expr->base.type = EXPR_UNARY_OP;
//     expr->operator = ACCEPTED;
//     expr->rhs = parse_expression();
//
//     if (expr->rhs == NULL) {
//       return NULL;
//     }
//
//     // @Lazy I know there's a cleaner way to handle this.
//     {
//       UnaryOpExpression* parent = expr;
//       AstExpression* subexpr = expr->rhs;
//       while (((UnaryOpExpression*) parent)->rhs->type == EXPR_BINARY_OP) {
//         parent = (void*) subexpr;
//         subexpr = parent->rhs;
//       }
//
//       if (parent != expr) {
//         result = expr->rhs;
//         parent->rhs = (AstExpression*) expr;
//         expr->rhs = subexpr;
//       } else {
//         result = (AstExpression*) expr;
//       }
//     }
//
//     return result;
//   } else {
//     error("Unable to parse expression");
//     return NULL;
//   }
//
//   if (accept(TOKEN_OPERATOR)) {
//     BinaryOpExpression* expr = malloc(sizeof(BinaryOpExpression));
//     expr->base.type = EXPR_BINARY_OP;
//     expr->operator = ACCEPTED;
//     expr->lhs = result;
//     expr->rhs = parse_expression();
//
//     if (expr->rhs == NULL) {
//       // @Leak expr
//       return NULL;
//     }
//
//     result = (AstExpression*) expr;
//   }
//
//   return result;
// }
//
// void* parse_directive() {
//   // @TODO Actually process this directive.
//   accept(TOKEN_DIRECTIVE);
//   return NULL;
// }
//
// void parse_namespace() {
//   while (TOKENS_REMAIN) {
//     if (accept(TOKEN_NEWLINE)) {
//       // Move on, nothing to see here.
//     } else if (test_directive()) {
//       void* directive = parse_directive();
//
//       // @TODO Actually implement these directives.
//       error("Hey, got a directive");
//     } else if (test_declaration()) {
//       AstDeclaration* decl = parse_declaration();
//       if (decl == NULL) {
//         error("Malformed declaration");
//         while (accept(TOKEN_NEWLINE));
//         continue;
//       }
//
//       if (!accept(TOKEN_NEWLINE)) {
//         error("Expected end of declaration");
//         continue;
//       }
//
//       // @TODO Figure this out better.
//       ParserScope* scope = CURRENT_SCOPE;
//       list_append(scope->nodes, decl);
//     } else {
//       error("Unrecognized code in top-level context");
//       while (accept(TOKEN_NEWLINE));
//     }
//   }
// }

//  TOP_LEVEL = DECLARATION
//            | TOP_LEVEL_DIRECTIVE    @TODO
void parse_top_level(ParserState* state) {
  while (tokens_remain(state)) {
    if (accept(state, TOKEN_NEWLINE)) {
      // Move on, nothing to see here.

//     } else if (test_directive()) {
//       void* directive = parse_directive();
//
//       // @TODO Actually implement these directives.
//       error("Hey, got a directive");

    } else if (test_declaration(state)) {
      AstNode* decl = pool_get(state->nodes);
      parse_declaration(state, decl);

      // if (decl == NULL) {
      //   error("Malformed declaration");
      //   while (accept(TOKEN_NEWLINE));
      //   continue;
      // }
      //
      // if (!accept(TOKEN_NEWLINE)) {
      //   error("Expected end of declaration");
      //   continue;
      // }

      // // @TODO Figure this out better.
      // ParserScope* scope = CURRENT_SCOPE;
      // list_append(scope->nodes, decl);

    } else {
      printf("Unrecognized code in top-level context\n");
      for (int i = state->pos; i < state->data.length; i++) {
        print_token(&state->data.tokens[i]); printf("\n");
      }
    }
  }
}

bool perform_parse_job(ParseJob* job) {
  ParserState* state = new_parser_state(job->tokens);
  for (int i = state->pos; i < state->data.length; i++) {
    print_token(&state->data.tokens[i]); printf("\n");
  }

  parse_top_level(state);

  return 1;
}

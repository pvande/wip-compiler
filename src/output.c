// int indentation = 0;
// #define INDENT  for (int __indent = 0; __indent < indentation; __indent++) printf("  ");
//
// void output_symbol(Symbol x) {
//   String* str = symbol_lookup(x);
//   for (int i = 0; i < str->length; i++) printf("%c", str->data[i]);
// }
//
// void output_mangled_name(Symbol name) {
//   printf("·"); output_symbol(name);
// }
//
// void output_procedure_declaration(ProcedureExpression* fn) {
//   // @TODO Bring back location identifying information for debugging.
//   output_symbol(fn->returns.name);
//   printf(" ");
//   output_mangled_name(fn->name);
//   printf("(");
//   for (int i = 0; i < fn->arguments->size; i++) {
//     if (i > 0) printf(", ");
//     AstDeclaration* arg = list_get(fn->arguments, i);
//     output_symbol(arg->type->name);
//     printf(" ");
//     output_symbol(arg->name);
//   }
//   printf(")");
// }
//
// void output_forward_variable_declaration(AstDeclaration* decl) {
//   // @TODO This.
// }
//
// void output_forward_declaration(AstDeclaration* decl) {
//   if (decl->value && decl->value->type == EXPR_PROCEDURE) {
//     output_procedure_declaration((ProcedureExpression*) decl->value);
//     printf(";\n");
//   } else {
//     output_forward_variable_declaration(decl);
//   }
// }
//
// void output_code_block(ProcedureExpression* fn);
// void output_expression(AstExpression* expr) {
//   if (expr->type == EXPR_LITERAL) {
//     LiteralExpression* lit = (LiteralExpression*) expr;{}
//     String x = lit->literal->source;
//     for (int i = 0; i < x.length; i++) printf("%c", x.data[i]);
//   } else if (expr->type == EXPR_CALL) {
//     CallExpression* call = (CallExpression*) expr;
//     output_symbol(call->procedure);
//     printf("(");
//     for (int i = 0; i < call->arguments->size; i++) {
//       if (i > 0) printf(", ");
//       AstExpression* e = list_get(call->arguments, i);
//       output_expression(e);
//     }
//     printf(")");
//   }
// }
//
// void output_code_block(ProcedureExpression* fn) {
//   printf("{\n");
//   indentation += 1;
//
//   // Output procedure body.
//   for (int i = 0; i < fn->body->size; i++) {
//     AstStatement* stmt = list_get(fn->body, i);
//
//     if (stmt->type == STATEMENT_DECLARATION) {
//       INDENT;
//       print_pointer(stmt->data);
//       printf("; // Declaration\n");
//     } else if (stmt->type == STATEMENT_EXPRESSION) {
//       INDENT;
//       output_expression(stmt->data);
//       printf("; // Expression\n");
//     }
//   }
//
//   indentation -= 1;
//   printf("}\n\n");
// }
//
// void output_c_code_for_declarations(List* declarations) {
//   // @TODO Inline base types and runtime support here.
//
//   printf("\n");
//   for (size_t i = 0; i < declarations->size; i++) {
//     AstDeclaration* decl = list_get(declarations, i);
//     output_forward_declaration(decl);
//   }
//
//   printf("\n");
//   for (size_t i = 0; i < declarations->size; i++) {
//     AstDeclaration* decl = list_get(declarations, i);
//     if (decl->value && decl->value->type == EXPR_PROCEDURE) {
//       ProcedureExpression* fn = (ProcedureExpression*) decl->value;
//       output_procedure_declaration(fn);
//       printf(" ");
//       output_code_block(fn);
//     }
//   }
//
//   printf("\n");
//   printf("int main() {\n");
//   printf("  ·main();\n");
//   printf("}\n");
// }

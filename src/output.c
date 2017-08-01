int indentation = 0;
#define INDENT  for (int __indent = 0; __indent < indentation; __indent++) printf("  ");

void output_symbol(Symbol x) {
  String* str = symbol_lookup(x);
  for (int i = 0; i < str->length; i++) printf("%c", str->data[i]);
}

void output_mangled_name(Symbol name) {
  printf("·"); output_symbol(name);
}

void output_function_declaration(AstDeclaration* decl) {
  FunctionExpression* fn = (FunctionExpression*) decl->value;

  // @TODO Bring back location identifying information for debugging.
  output_symbol(fn->returns.name);
  printf(" ");
  output_mangled_name(decl->name);
  printf("(");
  for (int i = 0; i < fn->arguments->size; i++) {
    if (i > 0) printf(", ");
    AstDeclaration* arg = list_get(fn->arguments, i);
    output_symbol(arg->type->name);
    printf(" ");
    output_symbol(arg->name);
  }
  printf(")");
}

void output_forward_variable_declaration(AstDeclaration* decl) {
  // @TODO This.
}

void output_forward_declaration(AstDeclaration* decl) {
  if (decl->value && decl->value->type == EXPR_FUNCTION) {
    output_function_declaration(decl);
    printf(";\n");
  } else {
    output_forward_variable_declaration(decl);
  }
}

void output_function_implementation(AstDeclaration* decl) {
  output_function_declaration(decl);
  printf(" {\n");
  indentation += 1;

  // Output function body.
  FunctionExpression* fn = (FunctionExpression*) decl->value;
  for (int i = 0; i < fn->body->size; i++) {
    AstStatement* stmt = list_get(fn->body, i);

    if (stmt->type == STATEMENT_DECLARATION) {
      INDENT;
      print_pointer(stmt->data);
      printf("; // Declaration\n");
    } else if (stmt->type == STATEMENT_EXPRESSION) {
      INDENT;
      print_pointer(stmt->data);
      printf("; // Expression\n");
    }
  }

  indentation -= 1;
  printf("}\n\n");
}

void output_c_code_for_declarations(List* declarations) {
  // @TODO Inline base types and runtime support here.

  printf("\n");
  for (size_t i = 0; i < declarations->size; i++) {
    AstDeclaration* decl = list_get(declarations, i);
    output_forward_declaration(decl);
  }

  printf("\n");
  for (size_t i = 0; i < declarations->size; i++) {
    AstDeclaration* decl = list_get(declarations, i);
    if (decl->value && decl->value->type == EXPR_FUNCTION) {
      output_function_implementation(decl);
    }
  }

  printf("\n");
  printf("int main() {\n");
  printf("  ·main();\n");
  printf("}\n");
}

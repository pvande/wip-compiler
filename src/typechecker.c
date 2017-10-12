const long long OVERFLOW_S8  = (1 << 7);
const long long OVERFLOW_U8  = (1 << 8);
const long long OVERFLOW_S16 = (1 << 15);
const long long OVERFLOW_U16 = (1 << 16);
const long long OVERFLOW_S32 = ((unsigned long long) 1 << 31);
const long long OVERFLOW_U32 = ((unsigned long long) 1 << 32);
const long long OVERFLOW_S64 = ((unsigned long long) 1 << 63);

// ** Constant Errors ** //

DEFINE_STR(ERR_UNDEFINED_TYPE, "Definition for type not found");
DEFINE_STR(ERR_UNDECLARED_IDENT, "Declaration for that identifier was not found");  // @TODO Less cryptic.
DEFINE_STR(ERR_COULD_NOT_INFER_TYPE, "Could not infer the type of this variable");  // @TODO Less cryptic.
DEFINE_STR(ERR_INCOMPATIBLE_TYPES, "Cannot assign that argument to that variable; the types don't match");  // @TODO Less cryptic.
DEFINE_STR(ERR_ARGUMENT_TYPE_MISMATCH, "No overload for that function takes those argument types");  // @TODO Less cryptic.
DEFINE_STR(ERR_UNHANDLED_LITERAL_TYPE, "Internal Compiler Error: Unhandled literal type");
DEFINE_STR(ERR_UNHANDLED_EXPRESSION_TYPE, "Internal Compiler Error: Unhandled expression type");
DEFINE_STR(ERR_UNHANDLED_NODE_TYPE, "Internal Compiler Error: Unhandled node type");


// ** Helpers ** //

AstNode* _find_identifier(Scope* scope, Symbol ident) {
  for (size_t i = 0; i < scope->declarations.length; i++) {
    AstNode* decl = list_get(&scope->declarations, i);
    if (decl->ident == ident) return decl;
  }

  if (scope->parent == NULL) {
    return NULL;
  } else {
    return _find_identifier(scope->parent, ident);
  }
}


// ** Typechecking ** //

bool typecheck_node(Job* job, AstNode* node);

bool typecheck_type(Job* job, AstNode* node) {
  Typeclass* type = type_find(job->ws, &node->source);

  if (type == NULL) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_UNDEFINED_TYPE;
    return 0;
  }

  node->typeclass = type;
  return 1;
}

bool typecheck_declaration(Job* job, AstNode* node) {
  AstNode* type = node->rhs;

  // This is the basic deferred type inference case.
  if (type == NULL) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_COULD_NOT_INFER_TYPE;
    return 1;
  }

  bool result = typecheck_node(job, type);

  node->typeclass = type->typeclass;
  if (node->typeclass == NULL) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = NULL;
  }

  return result;
}

bool typecheck_assignment(Job* job, AstNode* node) {
  AstNode* target = _find_identifier(node->scope, node->lhs->ident);
  AstNode* value = node->rhs;

  node->lhs = target;

  assert(target != NULL);
  assert(value != NULL);

  bool value_result = typecheck_node(job, value);
  bool target_result = typecheck_node(job, target);

  node->flags |= (value->flags & NODE_CONTAINS_ERROR);
  node->flags |= (target->flags & NODE_CONTAINS_ERROR);
  if ((value->flags & NODE_CONTAINS_ERROR) && !(value->flags & EXPR_IDENT)) {
    target->flags &= ~NODE_CONTAINS_ERROR;
  }

  if (!value_result) return 0;
  if (!target_result) return 0;
  if ((node->flags & NODE_CONTAINS_ERROR) && target->typeclass != NULL) return 1;

  if (target->typeclass == NULL) {
    target->typeclass = value->typeclass;
    target->flags &= ~NODE_CONTAINS_ERROR;
    target->error = NULL;

    node->typeclass = target->typeclass;
    node->flags &= (value->flags & NODE_CONTAINS_ERROR);
    return 1;
  } else {
    bool result = value->typeclass == target->typeclass;

    if (!result) {
      node->flags |= NODE_CONTAINS_ERROR;
      node->error = ERR_INCOMPATIBLE_TYPES;
      return 1;
    }

    if (value->typeclass->kind & KIND_LITERAL) {
      value->typeclass = target->typeclass;
    }

    node->typeclass = target->typeclass;
    return 1;
  }
}

bool typecheck_expression_identifier(Job* job, AstNode* node) {
  AstNode* decl = _find_identifier(node->scope, node->ident);

  if (decl == NULL) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_UNDECLARED_IDENT;
    return 0;
  }

  node->flags |= (decl->flags & NODE_CONTAINS_ERROR);

  if (decl->typeclass == NULL) return 0;

  // @TODO Type concretization should back propagate through intermediate
  //       variables.
  node->declaration = decl;
  node->typeclass = decl->typeclass;

  return 1;
}

DEFINE_STR(STR_LITERAL, "<literal>");
void _type_literal_number_value(AstNode* node) {
  // @TODO Describe all possible types.
  node->typeclass = type_create_untracked(STR_LITERAL, 0);
  node->typeclass->kind = KIND_LITERAL | KIND_NUMERIC;
}

bool typecheck_expression_literal_decimal(Job* job, AstNode* node) {
  node->int_value = 0;

  for (int i = 0; i < node->source.length; i++) {
    char c = node->source.data[i];
    if (c == '_') continue;

    node->int_value *= 10;
    node->int_value += c - '0';
  }

  _type_literal_number_value(node);

  return 1;
}

bool typecheck_expression_literal_hex(Job* job, AstNode* node) {
  node->int_value = 0;

  assert(node->source.data[0] == '0');
  assert(node->source.data[1] == 'x');

  for (int i = 2; i < node->source.length; i++) {
    char c = node->source.data[i];
    if (c == '_') continue;

    node->int_value *= 16;
    if (c >= '0' && c <= '9') {
      node->int_value += c - '0';
    } else if (c >= 'a' && c <= 'z') {
      node->int_value += 10 + c - 'a';
    } else {
      node->int_value += 10 + c - 'A';
    }
  }

  _type_literal_number_value(node);

  return 1;
}

bool typecheck_expression_literal_binary(Job* job, AstNode* node) {
  node->int_value = 0;

  assert(node->source.data[0] == '0');
  assert(node->source.data[1] == 'b');

  for (int i = 2; i < node->source.length; i++) {
    char c = node->source.data[i];
    if (c == '_') continue;

    node->int_value *= 2;
    node->int_value += c - '0';
  }

  _type_literal_number_value(node);

  return 1;
}

bool typecheck_expression_literal_fractional(Job* job, AstNode* node) {
  node->double_value = strtod(to_zero_terminated_string(&node->source), NULL);

  node->typeclass = type_find(job->ws, STR_FLOAT);

  return 1;
}

bool typecheck_expression_literal_string(Job* job, AstNode* node) {
  node->pointer_value = unescape_string_literal(&node->source);
  node->typeclass = type_find(job->ws, STR_STRING);

  return 1;
}

bool typecheck_expression_literal(Job* job, AstNode* node) {
  if (node->flags & IS_DECIMAL_LITERAL) {
    return typecheck_expression_literal_decimal(job, node);
  } else if (node->flags & IS_HEX_LITERAL) {
    return typecheck_expression_literal_hex(job, node);
  } else if (node->flags & IS_BINARY_LITERAL) {
    return typecheck_expression_literal_binary(job, node);
  } else if (node->flags & IS_FRACTIONAL_LITERAL) {
    return typecheck_expression_literal_fractional(job, node);
  } else if (node->flags & IS_STRING_LITERAL) {
    return typecheck_expression_literal_string(job, node);
  } else {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_UNHANDLED_LITERAL_TYPE;
    return 1;
  }
}

bool typecheck_expression_procedure(Job* job, AstNode* node) {
  AstNode* arguments = node->lhs;
  AstNode* returns = node->rhs;

  assert(arguments != NULL);
  assert(returns != NULL);
  assert(node->body_length == 1);
  assert(node->body != NULL);

  List* argument_types = new_list(1, arguments->body_length);
  List* return_types = new_list(1, returns->body_length);

  bool success = 1;
  size_t typestring_length = 8;  // "() => ()"

  for (size_t i = 0; i < arguments->body_length; i++) {
    success &= typecheck_node(job, &arguments->body[i]);
    arguments->flags |= (arguments->body[i].flags & NODE_CONTAINS_ERROR);
    node->flags |= (arguments->body[i].flags & NODE_CONTAINS_ERROR);

    if (success) {
      if (i > 0) typestring_length += 2;  // ", "
      typestring_length += arguments->body[i].typeclass->name->length;
      list_append(argument_types, arguments->body[i].typeclass);
    }
  }
  for (size_t i = 0; i < returns->body_length; i++) {
    success &= typecheck_node(job, &returns->body[i]);
    returns->flags |= (returns->body[i].flags & NODE_CONTAINS_ERROR);
    node->flags |= (returns->body[i].flags & NODE_CONTAINS_ERROR);

    if (success) {
      if (i > 0) typestring_length += 2;  // ", "
      typestring_length += returns->body[i].typeclass->name->length;
      list_append(return_types, returns->body[i].typeclass);
    }
  }

  if (success) {
    pipeline_emit_typecheck_job(job->ws, job->file, node->body);

    char* _name = malloc(typestring_length * sizeof(char));
    char* pos = _name + 1;
    _name[0] = '(';

    for (size_t i = 0; i < arguments->body_length; i++) {
      String* type_name = arguments->body[i].typeclass->name;

      if (i > 0) {
        pos[0] = ',';
        pos[1] = ' ';
        pos += 2;
      }

      memcpy(pos, type_name->data, type_name->length);
      pos += type_name->length;
    }

    pos[0] = ')';
    pos[1] = ' ';
    pos[2] = '=';
    pos[3] = '>';
    pos[4] = ' ';
    pos[5] = '(';
    pos += 6;

    for (size_t i = 0; i < returns->body_length; i++) {
      String* type_name = returns->body[i].typeclass->name;

      if (i > 0) {
        pos[0] = ',';
        pos[1] = ' ';
        pos += 2;
      }

      memcpy(pos, type_name->data, type_name->length);
      pos += type_name->length;
    }

    pos[0] = ')';

    String* name = malloc(sizeof(String));
    name->length = typestring_length;
    name->data = _name;

    node->typeclass = type_find(job->ws, name);

    if (node->typeclass == NULL) {
      node->typeclass = type_create(job->ws, name, 64);
      node->typeclass->from = argument_types;
      node->typeclass->to = return_types;
    }

    // @TODO Maybe don't allocate these unless they're necessary?
    free_list(argument_types);
    free_list(return_types);

    // @Leak: String data is never reclaimed.
    // @Leak: String container is never reclaimed.
  }

  return success;
}

bool typecheck_expression_call(Job* job, AstNode* node) {
  AstNode* decl = _find_identifier(node->scope, node->ident);

  if (decl == NULL) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_UNDECLARED_IDENT;
    return 0;
  }

  if (decl->typeclass == NULL) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_COULD_NOT_INFER_TYPE;
    return 0;
  }

  assert(decl->typeclass->from != NULL);
  assert(decl->typeclass->to != NULL);

  bool success = 1;
  for (size_t i = 0; i < node->rhs->body_length; i++) {
    AstNode* arg = &node->rhs->body[i];

    success &= typecheck_node(job, arg);
    node->rhs->flags |= (arg->flags & NODE_CONTAINS_ERROR);
  }
  node->flags |= (node->rhs->flags & NODE_CONTAINS_ERROR);
  if (!success) return 0;

  List* arg_types = decl->typeclass->from;
  if (node->rhs->body_length != arg_types->length) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_ARGUMENT_TYPE_MISMATCH;
    return 0;
  }

  for (size_t i = 0; i < arg_types->length; i++){
    AstNode* arg = &node->rhs->body[i];
    Typeclass* arg_type = list_get(arg_types, i);

    if (arg->typeclass != arg_type) {
      node->flags |= NODE_CONTAINS_ERROR;
      node->rhs->flags |= NODE_CONTAINS_ERROR;
      arg->flags |= NODE_CONTAINS_ERROR;
      arg->error = ERR_ARGUMENT_TYPE_MISMATCH;
      return 0;
    }
  }

  node->declaration = decl;
  if (decl->typeclass->to->length > 0) {
    node->typeclass = list_get(decl->typeclass->to, 0);
  } else {
    node->typeclass = type_find(job->ws, STR_VOID);
  }

  return 1;
}

bool typecheck_return(Job* job, AstNode* node) {
  // @TODO We need to know what function we're returning from in order to make
  //       sure that the return type matches the function's type.
  bool result = 1;

  if (node->flags & NODE_CONTAINS_RHS) {
    result = typecheck_node(job, node->rhs);

    if (result) {
      node->typeclass = node->rhs->typeclass;
    }
  } else {
    node->typeclass = type_find(job->ws, STR_VOID);
  }

  return result;
}

bool typecheck_expression(Job* job, AstNode* node) {
  if (node->flags & EXPR_IDENT) {
    return typecheck_expression_identifier(job, node);
  } else if (node->flags & EXPR_LITERAL) {
    return typecheck_expression_literal(job, node);
  } else if (node->flags & EXPR_PROCEDURE) {
    return typecheck_expression_procedure(job, node);
  } else if (node->flags & EXPR_CALL) {
    return typecheck_expression_call(job, node);
  } else {
    node->flags |= NODE_CONTAINS_ERROR;
    node->error = ERR_UNHANDLED_EXPRESSION_TYPE;
    return 1;
  }
}

bool typecheck_compound(Job* job, AstNode* node) {
  bool result = 1;

  for (size_t i = 0; i < node->body_length; i++) {
    AstNode* child = &node->body[i];
    result &= typecheck_node(job, child);
    node->flags |= (child->flags & NODE_CONTAINS_ERROR);
  }

  if (result) {
    node->typeclass = type_find(job->ws, STR_VOID);
  }

  return result;
}

bool typecheck_conditional(Job* job, AstNode* node) {
  bool result = 1;
  AstNode* condition = node->lhs;
  AstNode* body = node->body;

  result = typecheck_node(job, condition);
  if (!result) return result;

  if (condition->typeclass != type_find(job->ws, STR_BOOL)) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->lhs->flags |= NODE_CONTAINS_ERROR;
    condition->flags |= NODE_CONTAINS_ERROR;
    condition->error = ERR_INCOMPATIBLE_TYPES;
    return 0;
  }

  result = typecheck_node(job, body);
  node->typeclass = type_find(job->ws, STR_VOID);

  return result;
}

bool typecheck_loop(Job* job, AstNode* node) {
  bool result = 1;
  AstNode* block = node->body;

  result = typecheck_node(job, block);
  node->typeclass = type_find(job->ws, STR_VOID);

  return result;
}

bool typecheck_node(Job* job, AstNode* node) {
  node->flags &= ~NODE_CONTAINS_ERROR;
  if (node->typeclass != NULL) return 1;
  bool result;

  // printf("typecheck_node -> "); inspect_ast_node(node); printf("\n");
  switch (node->type) {
    case NODE_TYPE:
      result = typecheck_type(job, node);
      break;
    case NODE_DECLARATION:
      result = typecheck_declaration(job, node);
      break;
    case NODE_ASSIGNMENT:
      result = typecheck_assignment(job, node);
      break;
    case NODE_EXPRESSION:
      result = typecheck_expression(job, node);
      break;
    case NODE_COMPOUND:
      result = typecheck_compound(job, node);
      break;
    case NODE_RETURN:
      result = typecheck_return(job, node);
      break;
    case NODE_CONDITIONAL:
      result = typecheck_conditional(job, node);
      break;
    case NODE_LOOP:
      result = typecheck_loop(job, node);
      break;
    case NODE_BREAK:
      node->typeclass = type_find(job->ws, STR_VOID);
      result = 1;
      break;
    default:
      node->flags |= NODE_CONTAINS_ERROR;
      node->error = ERR_UNHANDLED_NODE_TYPE;
      result = 1;
  }
  // printf("typecheck_node <- "); inspect_ast_node(node); printf("\n");

  return result;
}

bool perform_typecheck_job(Job* job) {
  bool result = typecheck_node(job, job->node);

  // printf("«««««««»»»»»»»\n");
  // print_ast_node_as_tree(job->file->lines, job->node);
  // printf("«««««««»»»»»»»\n");

  if (result) {
    if (job->node->flags & NODE_CONTAINS_ERROR) {
      pipeline_emit_abort_job(job->ws, job->file, job->node);
    } else {
      pipeline_emit_optimize_job(job->ws, job->file, job->node);
    }
  }

  return result;
}

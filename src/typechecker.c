Pool TYPECLASS_POOL;
Table TYPECLASS_TABLE;

Typeclass TYPECLASS_LITERAL = { -1, 0, &(String) { 11, "«LITERAL»" }, NULL, NULL };

const long long OVERFLOW_S8  = (1 << 7);
const long long OVERFLOW_U8  = (1 << 8);
const long long OVERFLOW_S16 = (1 << 15);
const long long OVERFLOW_U16 = (1 << 16);
const long long OVERFLOW_S32 = ((unsigned long long) 1 << 31);
const long long OVERFLOW_U32 = ((unsigned long long) 1 << 32);
const long long OVERFLOW_S64 = ((unsigned long long) 1 << 63);

void* _get_type(String* name) {
  return table_find(&TYPECLASS_TABLE, name);
}

void* _new_type(String* name, size_t size) {
  static size_t serial = 0;

  Typeclass* type = pool_get(&TYPECLASS_POOL);
  type->id = serial++;
  type->size = size;
  type->name = name;
  type->from = NULL;
  type->to = NULL;

  table_add(&TYPECLASS_TABLE, name, type);

  return type;
}

void* _find_or_create_type(String* name, size_t size) {
  // @Concurrency This is not atomic!
  Typeclass* type = _get_type(name);

  if (type == NULL) {
    // @TODO Clone the name string?
    type = _new_type(name, size);
  } else {
    if (type->size != size) {
      fprintf(stderr, "Internal Compiler Error: Attempted to create type '%s' with different sizes (%zu & %zu)!", to_zero_terminated_string(name), size, type->size);
      exit(1);
    }
  }

  return type;
}

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

// @TODO Implement (lossless) type compatibility.
//       For example, a u8 can always "fit" into a u16.
bool typecheck_can_coerce(Typeclass* from, Typekind kind, Typeclass* to) {
  if (from == to) return 1;

  if (from == &TYPECLASS_LITERAL) {
    if (to->name == STR_BOOL) {
      return kind & KIND_NUMBER;
    }

    if (to->name == STR_INT)
      return kind & (KIND_CAN_BE_U64 | KIND_CAN_BE_SIGNED);

    if (to->name == STR_S64)
      return kind & (KIND_CAN_BE_U64 | KIND_CAN_BE_SIGNED);

    if (to->name == STR_S32)
      return kind & (KIND_CAN_BE_U32 | KIND_CAN_BE_SIGNED);

    if (to->name == STR_S16)
      return kind & (KIND_CAN_BE_U16 | KIND_CAN_BE_SIGNED);

    if (to->name == STR_S8)
      return kind & (KIND_CAN_BE_U8 | KIND_CAN_BE_SIGNED);

    if (to->name == STR_U64)
      return kind & KIND_CAN_BE_U64;

    if (to->name == STR_U32)
      return kind & KIND_CAN_BE_U32;

    if (to->name == STR_U16)
      return kind & KIND_CAN_BE_U16;

    if (to->name == STR_U8)
      return kind & KIND_CAN_BE_U8;

    if (to->name == STR_BYTE) {
      return kind & KIND_CAN_BE_U8;
    }

    return 0;
  }

  // "int" is an alias for "s64"
  if (from->name == STR_INT && to->name == STR_S64) return 1;
  if (from->name == STR_S64 && to->name == STR_INT) return 1;

  // "byte" is an alias for "u8"
  if (from->name == STR_BYTE && to->name == STR_U8) return 1;
  if (from->name == STR_U8 && to->name == STR_BYTE) return 1;

  return 0;
}


// ** Typechecking ** //

bool typecheck_node(Job* job, AstNode* node);


bool typecheck_type(Job* job, AstNode* node) {
  Typeclass* type = _get_type(&node->source);

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
  AstNode* target = node->lhs;
  AstNode* value = node->rhs;

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
    target->typekind = value->typekind;
    target->flags &= ~NODE_CONTAINS_ERROR;
    target->error = NULL;

    node->typeclass = target->typeclass;
    node->typekind = target->typekind;
    node->flags &= (value->flags & NODE_CONTAINS_ERROR);
    return 1;
  } else {
    bool result = typecheck_can_coerce(value->typeclass, value->typekind, target->typeclass);

    if (!result) {
      node->flags |= NODE_CONTAINS_ERROR;
      node->error = ERR_INCOMPATIBLE_TYPES;
      return 1;
    }

    if (value->typeclass == &TYPECLASS_LITERAL) {
      value->typeclass = target->typeclass;
    }

    node->typeclass = target->typeclass;
    node->typekind = target->typekind;
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
  node->typekind = decl->typekind;

  return 1;
}

void _type_literal_number_value(AstNode* node) {
  // @TODO Describe all possible types.
  node->typeclass = &TYPECLASS_LITERAL;
  node->typekind = KIND_NUMBER;

  if (node->int_value < OVERFLOW_U8) {
    node->typekind |= KIND_CAN_BE_U8;
    node->typekind |= KIND_CAN_BE_U16;
    node->typekind |= KIND_CAN_BE_U32;
    node->typekind |= KIND_CAN_BE_U64;

    if (node->int_value < OVERFLOW_S8) {
      node->typekind |= KIND_CAN_BE_SIGNED;
    }

  } else if (node->int_value < OVERFLOW_U16) {
    node->typekind |= KIND_CAN_BE_U16;
    node->typekind |= KIND_CAN_BE_U32;
    node->typekind |= KIND_CAN_BE_U64;

    if (node->int_value < OVERFLOW_S16) {
      node->typekind |= KIND_CAN_BE_SIGNED;
    }

  } else if (node->int_value < OVERFLOW_U32) {
    node->typekind |= KIND_CAN_BE_U32;
    node->typekind |= KIND_CAN_BE_U64;

    if (node->int_value < OVERFLOW_S32) {
      node->typekind |= KIND_CAN_BE_SIGNED;
    }

  } else {
    node->typekind |= KIND_CAN_BE_U64;

    if (node->int_value < OVERFLOW_S64) {
      node->typekind |= KIND_CAN_BE_SIGNED;
    }
  }
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

  node->typeclass = _get_type(STR_FLOAT);
  node->typekind = KIND_NUMBER;

  return 1;
}

bool typecheck_expression_literal_string(Job* job, AstNode* node) {
  node->pointer_value = unescape_string_literal(&node->source);
  node->typeclass = _get_type(STR_STRING);

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

    node->typeclass = _find_or_create_type(name, 64);
    if (node->typeclass->from == NULL) {
      node->typeclass->from = argument_types;
      node->typeclass->to = return_types;
    } else {
      // @TODO Maybe don't allocate these unless they're necessary?
      free_list(argument_types);
      free_list(return_types);
    }

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

    if (!typecheck_can_coerce(arg->typeclass, arg->typekind, arg_type)) {
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
    node->typeclass = _get_type(STR_VOID);
  }

  return 1;
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
    node->typeclass = _get_type(STR_VOID);
  }

  return result;
}

bool typecheck_conditional(Job* job, AstNode* node) {
  bool result = 1;
  AstNode* condition = node->lhs;
  AstNode* body = node->body;

  result = typecheck_node(job, condition);
  if (!result) return result;

  if (!typecheck_can_coerce(condition->typeclass, condition->typekind, _get_type(STR_BOOL))) {
    node->flags |= NODE_CONTAINS_ERROR;
    node->lhs->flags |= NODE_CONTAINS_ERROR;
    condition->flags |= NODE_CONTAINS_ERROR;
    condition->error = ERR_INCOMPATIBLE_TYPES;
    return 0;
  }

  result = typecheck_node(job, body);
  node->typeclass = _get_type(STR_VOID);

  return result;
}

bool typecheck_loop(Job* job, AstNode* node) {
  bool result = 1;
  AstNode* block = node->body;

  result = typecheck_node(job, block);
  node->typeclass = _get_type(STR_VOID);

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
    case NODE_CONDITIONAL:
      result = typecheck_conditional(job, node);
      break;
    case NODE_LOOP:
      result = typecheck_loop(job, node);
      break;
    case NODE_BREAK:
      node->typeclass = _get_type(STR_VOID);
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


void initialize_typechecker() {
  initialize_pool(&TYPECLASS_POOL, sizeof(Typeclass), 8, 32);
  initialize_table(&TYPECLASS_TABLE, 256);

  _new_type(STR_VOID, 0);
  _new_type(STR_BOOL, 8);
  _new_type(STR_BYTE, 8);
  _new_type(STR_U8,   8);
  _new_type(STR_U16,  16);
  _new_type(STR_U32,  32);
  _new_type(STR_U64,  64);
  _new_type(STR_S8,   8);
  _new_type(STR_S16,  16);
  _new_type(STR_S32,  32);
  _new_type(STR_S64,  64);
  _new_type(STR_INT,  64);
  _new_type(STR_FLOAT, 64);
  _new_type(STR_STRING, 64);
}

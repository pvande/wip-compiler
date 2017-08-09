Pool TYPECLASS_POOL;
Table TYPECLASS_TABLE;

Typeclass TYPECLASS_LITERAL;
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

AstNode* _find_identifier(Scope* scope, Symbol ident) {
  for (size_t i = 0; i < scope->declarations->length; i++) {
    AstNode* decl = list_get(scope->declarations, i);
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
bool typecheck_can_coerce(AstNode* from, AstNode* to) {
  if (from->typeclass == to->typeclass) return 1;

  if (from->typeclass == &TYPECLASS_LITERAL) {
    if (to->typeclass->name == STR_INT)
      return from->typekind & (KIND_CAN_BE_U64 | KIND_CAN_BE_SIGNED);

    if (to->typeclass->name == STR_S64)
      return from->typekind & (KIND_CAN_BE_U64 | KIND_CAN_BE_SIGNED);

    if (to->typeclass->name == STR_S32)
      return from->typekind & (KIND_CAN_BE_U32 | KIND_CAN_BE_SIGNED);

    if (to->typeclass->name == STR_S16)
      return from->typekind & (KIND_CAN_BE_U16 | KIND_CAN_BE_SIGNED);

    if (to->typeclass->name == STR_S8)
      return from->typekind & (KIND_CAN_BE_U8 | KIND_CAN_BE_SIGNED);

    if (to->typeclass->name == STR_U64)
      return from->typekind & KIND_CAN_BE_U64;

    if (to->typeclass->name == STR_U32)
      return from->typekind & KIND_CAN_BE_U32;

    if (to->typeclass->name == STR_U16)
      return from->typekind & KIND_CAN_BE_U16;

    if (to->typeclass->name == STR_U8)
      return from->typekind & KIND_CAN_BE_U8;

    if (to->typeclass->name == STR_BYTE) {
      return from->typekind & KIND_CAN_BE_U8;
    }

    return 0;
  }

  // "int" is an alias for "s64"
  if (from->typeclass->name == STR_INT && to->typeclass->name == STR_S64) return 1;
  if (from->typeclass->name == STR_S64 && to->typeclass->name == STR_INT) return 1;

  // "byte" is an alias for "u8"
  if (from->typeclass->name == STR_BYTE && to->typeclass->name == STR_U8) return 1;
  if (from->typeclass->name == STR_U8 && to->typeclass->name == STR_BYTE) return 1;

  return 0;
}


bool typecheck_node(AstNode* node);


bool typecheck_type(AstNode* node) {
  Typeclass* type = _get_type(&node->source);

  if (type == NULL) {
    printf("Unknown type '%s'\n", to_zero_terminated_string(&node->source));
    return 0;
  }

  node->typeclass = type;
  return 1;
}

bool typecheck_declaration(AstNode* node) {
  AstNode* type = node->rhs;

  if (type == NULL) return 0;

  bool result = typecheck_node(type);
  if (!result) return 0;

  node->typeclass = type->typeclass;
  return 1;
}

bool typecheck_assignment(AstNode* node) {
  AstNode* target = node->lhs;
  AstNode* value = node->rhs;

  assert(target != NULL);
  assert(value != NULL);

  bool result = typecheck_node(value);
  if (!result) return 0;

  typecheck_node(target);

  if (target->typeclass == NULL) {
    target->typeclass = value->typeclass;
    target->typekind = value->typekind;
    return 1;
  } else {
    bool result = typecheck_can_coerce(value, target);

    if (result && value->typeclass == &TYPECLASS_LITERAL) {
      value->typeclass = target->typeclass;
    }

    return result;
  }
}

bool typecheck_expression_identifier(AstNode* node) {
  AstNode* decl = _find_identifier(node->scope, node->ident);
  if (decl == NULL) return 0;
  if (decl->typeclass == NULL) return 0;

  node->typeclass = decl->typeclass;
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

bool typecheck_expression_literal_decimal(AstNode* node) {
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

bool typecheck_expression_literal_hex(AstNode* node) {
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

bool typecheck_expression_literal_binary(AstNode* node) {
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

bool typecheck_expression_literal_fractional(AstNode* node) {
  node->double_value = strtod(to_zero_terminated_string(&node->source), NULL);

  node->typeclass = _get_type(STR_FLOAT);
  node->typekind = KIND_NUMBER;

  return 1;
}

bool typecheck_expression_literal_string(AstNode* node) {
  String* str = malloc(sizeof(String));
  str->data = malloc(node->source.length * sizeof(char));

  assert(node->source.data[0] == '"');
  assert(node->source.data[node->source.length - 1] == '"');

  size_t idx = 0;
  for (size_t i = 0; i < node->source.length - 2; i++) {
    char c = node->source.data[i + 1];

    if (c == '\\') {
      i += 1;
      switch (node->source.data[i + 1]) {
        case '0': str->data[idx++] = '\0'; break;
        case 'a': str->data[idx++] = '\a'; break;
        case 'b': str->data[idx++] = '\b'; break;
        case 't': str->data[idx++] = '\t'; break;
        case 'n': str->data[idx++] = '\n'; break;
        case 'v': str->data[idx++] = '\v'; break;
        case 'f': str->data[idx++] = '\f'; break;
        case 'r': str->data[idx++] = '\r'; break;
        case 'e': str->data[idx++] = '\e'; break;
        case '"': str->data[idx++] = '\"'; break;
        case '\\': str->data[idx++] = '\\'; break;
        default:
          printf("Unknown escape sequence: '\\%c'\n", node->source.data[i + 1]);
          return 0;
      }
    } else {
      str->data[idx++] = c;
    }
  }
  str->length = idx;

  node->pointer_value = str;
  node->typeclass = _get_type(STR_STRING);

  return 1;
}

bool typecheck_expression_literal(AstNode* node) {
  if (node->flags & IS_DECIMAL_LITERAL) {
    return typecheck_expression_literal_decimal(node);
  } else if (node->flags & IS_HEX_LITERAL) {
    return typecheck_expression_literal_hex(node);
  } else if (node->flags & IS_BINARY_LITERAL) {
    return typecheck_expression_literal_binary(node);
  } else if (node->flags & IS_FRACTIONAL_LITERAL) {
    return typecheck_expression_literal_fractional(node);
  } else if (node->flags & IS_STRING_LITERAL) {
    return typecheck_expression_literal_string(node);
  } else {
    printf("Unable to typecheck literal expression with flags %x\n", node->flags);
    return 0;
  }
}

bool typecheck_expression_procedure(AstNode* node) {
  AstNode* arguments = node->lhs;
  AstNode* returns = node->rhs;

  assert(arguments != NULL);
  assert(returns != NULL);
  assert(node->body_length == 1);
  assert(node->body != NULL);

  bool success = 1;
  size_t typestring_length = 8;  // "() => ()"

  for (size_t i = 0; i < arguments->body_length; i++) {
    success &= typecheck_node(&arguments->body[i]);

    if (success) {
      if (i > 0) typestring_length += 2;  // ", "
      typestring_length += arguments->body[i].typeclass->name->length;
    }
  }

  for (size_t i = 0; i < returns->body_length; i++) {
    success &= typecheck_node(&returns->body[i]);

    if (success) {
      if (i > 0) typestring_length += 2;  // ", "
      typestring_length += returns->body[i].typeclass->name->length;
    }
  }

  if (success) {
    pipeline_emit_typecheck_job(node->body);

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

    // @Leak: String data is never reclaimed.
    // @Leak: String container is never reclaimed.
  }

  return success;
}

bool typecheck_expression(AstNode* node) {
  if (node->flags & EXPR_IDENT) {
    return typecheck_expression_identifier(node);
  } else if (node->flags & EXPR_LITERAL) {
    return typecheck_expression_literal(node);
  } else if (node->flags & EXPR_PROCEDURE) {
    return typecheck_expression_procedure(node);
  } else {
    printf("Unable to typecheck unhandled expression with flags %x\n", node->flags);
    return 0;
  }
}

bool typecheck_compound(AstNode* node) {
  bool result = 1;

  for (size_t i = 0; i < node->body_length; i++) {
    AstNode* child = &node->body[i];
    result &= typecheck_node(child);

    if (!result) break;
  }

  return result;
}

bool typecheck_node(AstNode* node) {
  if (node->typeclass != NULL) return 1;
  switch (node->type) {
    case NODE_TYPE:
      return typecheck_type(node);
    case NODE_DECLARATION:
      return typecheck_declaration(node);
    case NODE_ASSIGNMENT:
      return typecheck_assignment(node);
    case NODE_EXPRESSION:
      return typecheck_expression(node);
    case NODE_COMPOUND:
      return typecheck_compound(node);
    default:
      printf("Unable to typecheck unhandled node type: %s\n", _ast_node_type(node));
      return 0;
  }
}

bool perform_typecheck_job(TypecheckJob* job) {
  return typecheck_node(job->declaration);
}


void initialize_typechecker() {
  initialize_pool(&TYPECLASS_POOL, sizeof(Typeclass), 8, 32);
  initialize_table(&TYPECLASS_TABLE, 256);

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

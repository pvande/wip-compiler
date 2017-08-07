Pool TYPECLASS_POOL;
Table TYPECLASS_TABLE;

void* _get_type(String* name) {
  return table_find(&TYPECLASS_TABLE, name);
}

void _new_type(String* name, size_t size) {
  static size_t serial = 0;

  Typeclass* type = pool_get(&TYPECLASS_POOL);
  type->id = serial++;
  type->size = size;
  type->name = name;

  table_add(&TYPECLASS_TABLE, name, type);
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


bool typecheck_node(AstNode* node);


bool typecheck_type_node(AstNode* node) {
  Typeclass* type = _get_type(&node->source);

  if (type == NULL) {
    printf("Unknown type '%s'\n", to_zero_terminated_string(&node->source));
    return 0;
  }

  node->typeclass = type;
  return 1;
}

bool typecheck_declaration_node(AstNode* node) {
  AstNode* type = node->rhs;

  if (type == NULL) return 0;

  bool result = typecheck_node(type);
  if (!result) return 0;

  node->typeclass = type->typeclass;
  return 1;
}

bool typecheck_assignment_node(AstNode* node) {
  AstNode* target = node->lhs;
  AstNode* value = node->rhs;

  assert(target != NULL);
  assert(value != NULL);

  bool result = typecheck_node(value);
  if (!result) return 0;

  typecheck_node(target);

  if (target->typeclass == NULL) {
    target->typeclass = value->typeclass;
    return 1;
  } else {
    // @TODO Implement (lossless) type compatibility.
    //       For example, a u8 can always "fit" into a u16.
    return target->typeclass == value->typeclass;
  }
}

bool typecheck_expression_identifier_node(AstNode* node) {
  AstNode* decl = _find_identifier(node->scope, node->ident);
  if (decl == NULL) return 0;
  if (decl->typeclass == NULL) return 0;

  node->typeclass = decl->typeclass;
  return 1;
}

bool typecheck_expression_node(AstNode* node) {
  if (node->flags & EXPR_IDENT) {
    return typecheck_expression_identifier_node(node);
  } else {
    printf("Unable to typecheck unhandled expression with flags %x\n", node->flags);
    return 0;
  }
}

bool typecheck_node(AstNode* node) {
  if (node->typeclass != NULL) return 1;
  switch (node->type) {
    case NODE_TYPE:
      return typecheck_type_node(node);
    case NODE_DECLARATION:
      return typecheck_declaration_node(node);
    case NODE_ASSIGNMENT:
      return typecheck_assignment_node(node);
    case NODE_EXPRESSION:
      return typecheck_expression_node(node);
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
}

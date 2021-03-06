#define EMIT(V)          (*((size_t*) pool_get(instructions)) = V)
#define LOAD(DECL)       do { EMIT(BC_LOAD); EMIT((size_t) DECL); } while (0);
#define LOAD_ARG(N)      do { EMIT(BC_ARG_LOAD); EMIT((size_t) N); } while (0);
#define STORE(DECL)      do { EMIT(BC_STORE); EMIT((size_t) DECL); } while (0);
#define PUSH(V)          do { EMIT(BC_PUSH); EMIT(V); } while (0);
#define CALL(DECL, LEN)  do { EMIT(BC_CALL); EMIT((size_t) DECL); EMIT((size_t) LEN); } while (0);
#define JUMP(TO)         do { EMIT(BC_JUMP); EMIT((size_t) TO); } while (0);
#define JUMP_ZERO(TO)    do { EMIT(BC_JUMP_ZERO); EMIT((size_t) TO); } while (0);
#define BREAK()          do { EMIT(BC_BREAK); EMIT(BC_BREAK); } while (0);

bool bytecode_handle_node(Pool* instructions, AstNode* node);

bool bytecode_handle_declaration(Pool* instructions, AstNode* node) {
  return 1;
}

bool bytecode_handle_assignment(Pool* instructions, AstNode* node) {
  AstNode* decl = node->lhs;
  AstNode* value = node->rhs;

  bool result = bytecode_handle_node(instructions, value);

  if (result) STORE(decl);

  return result;
}

bool bytecode_handle_expression_identifier(Pool* instructions, AstNode* node) {
  AstNode* decl = node->declaration;

  if (decl->flags & DECL_ARGUMENT) {
    LOAD_ARG(decl->int_value);
  } else {
    LOAD(decl);
  }

  return 1;
}

bool bytecode_handle_expression_integer(Pool* instructions, AstNode* node) {
  PUSH(node->int_value);
  return 1;
}

bool bytecode_handle_expression_float(Pool* instructions, AstNode* node) {
  PUSH(node->double_value);
  return 1;
}

bool bytecode_handle_expression_string(Pool* instructions, AstNode* node) {
  PUSH((size_t) node->pointer_value);
  return 1;
}

bool bytecode_handle_expression_literal(Pool* instructions, AstNode* node) {
  if (node->typeclass->kind & KIND_NUMERIC) {
    return bytecode_handle_expression_integer(instructions, node);
  } else if (node->typeclass->name == STR_FLOAT) {
    return bytecode_handle_expression_float(instructions, node);
  } else if (node->typeclass->name == STR_STRING) {
    return bytecode_handle_expression_string(instructions, node);
  } else {
    assert(0);
    return 0;
  }
}

bool bytecode_handle_expression_procedure(Pool* instructions, AstNode* node) {
  if (node->body[0].bytecode_id == -1) return 0;

  PUSH((size_t) node->body);
  return 1;
}

bool bytecode_handle_expression_call(Pool* instructions, AstNode* node) {
  AstNode* decl = node->declaration;
  AstNode* args = node->rhs;

  bool result = 1;

  for (size_t i = 0; i < args->body_length; i++) {
    result &= bytecode_handle_node(instructions, &args->body[args->body_length - i - 1]);
    if (!result) break;
  }

  if (result) CALL(decl, args->body_length);

  return result;
}

bool bytecode_handle_expression(Pool* instructions, AstNode* node) {
  if (node->flags & EXPR_IDENT) {
    return bytecode_handle_expression_identifier(instructions, node);
  } else if (node->flags & EXPR_LITERAL) {
    return bytecode_handle_expression_literal(instructions, node);
  } else if (node->flags & EXPR_PROCEDURE) {
    return bytecode_handle_expression_procedure(instructions, node);
  } else if (node->flags & EXPR_CALL) {
    return bytecode_handle_expression_call(instructions, node);
  } else {
    assert(0);
    return 0;
  }
}

bool bytecode_handle_compound(Pool* instructions, AstNode* node) {
  bool result = 1;

  for (size_t i = 0; i < node->body_length; i++) {
    result &= bytecode_handle_node(instructions, &node->body[i]);
    if (!result) break;
  }

  return result;
}

bool bytecode_handle_return(Pool* instructions, AstNode* node) {
  bool result = 1;
  AstNode* retval = node->rhs;

  if (node->flags & NODE_CONTAINS_RHS) {
    result = bytecode_handle_node(instructions, retval);
    EMIT(BC_RETURN);
    EMIT(1);
  } else {
    EMIT(BC_RETURN);
    EMIT(0);
  }

  return result;
}

bool bytecode_handle_conditional(Pool* instructions, AstNode* node) {
  bool result = 1;
  AstNode* cond = node->lhs;
  AstNode* branch = node->body;

  result = bytecode_handle_node(instructions, cond);
  if (!result) return result;

  Pool bytecode;  // @Leak The contained structures are never released.
  initialize_pool(&bytecode, sizeof(size_t), 16, 64);

  result = bytecode_handle_node(&bytecode, branch);
  if (!result) return result;

  JUMP_ZERO(bytecode.length);

  size_t* code = pool_to_array(&bytecode);
  for (size_t i = 0; i < bytecode.length; i++) EMIT(code[i]);
  free(code);

  return result;
}

bool bytecode_handle_loop(Pool* instructions, AstNode* node) {
  bool result = 1;
  AstNode* block = node->body;

  Pool bytecode;  // @Leak The contained structures are never released.
  initialize_pool(&bytecode, sizeof(size_t), 16, 64);

  result = bytecode_handle_node(&bytecode, block);
  if (!result) return result;

  size_t block_length = bytecode.length;

  size_t* code = pool_to_array(&bytecode);
  size_t* ptr = code;
  size_t* end_of_code = code + block_length;
  size_t offset = 0;
  while (ptr < end_of_code) {
    if (*ptr == BC_BREAK) {
      ptr[0] = BC_JUMP;
      ptr[1] = end_of_code - code;
    }

    // @MAYBE BC_RETRY?

    int count = BytecodeSizes[*ptr];
    for (int i = 0; i < count; i++) EMIT(ptr[i]);
    ptr += count;
  }
  free(code);

  JUMP(0 - (block_length + BytecodeSizes[BC_JUMP]));

  return result;
}

bool bytecode_handle_break(Pool* instructions, AstNode* node) {
  bool result = 1;

  BREAK();

  return result;
}

bool bytecode_handle_node(Pool* instructions, AstNode* node) {
  if (node->typeclass == NULL) return 0;

  bool result;

  switch (node->type) {
    case NODE_DECLARATION:
      result = bytecode_handle_declaration(instructions, node);
      break;
    case NODE_ASSIGNMENT:
      result = bytecode_handle_assignment(instructions, node);
      break;
    case NODE_EXPRESSION:
      result = bytecode_handle_expression(instructions, node);
      break;
    case NODE_COMPOUND:
      result = bytecode_handle_compound(instructions, node);
      break;
    case NODE_RETURN:
      result = bytecode_handle_return(instructions, node);
      break;
    case NODE_CONDITIONAL:
      result = bytecode_handle_conditional(instructions, node);
      break;
    case NODE_LOOP:
      result = bytecode_handle_loop(instructions, node);
      break;
    case NODE_BREAK:
      result = bytecode_handle_break(instructions, node);
      break;
    default:
      assert(0);
      result = 0;
  }

  return result;
}

bool bytecode_handle_top_level_node(CompilationWorkspace* ws, AstNode* node) {
  if (node->type == NODE_DECLARATION) return 1;

  Pool bytecode;  // @Leak The contained structures are never released.
  initialize_pool(&bytecode, sizeof(size_t), 16, 64);

  assert(node->type == NODE_ASSIGNMENT || node->type == NODE_COMPOUND);
  bool result = bytecode_handle_node(&bytecode, node);
  if (result) {
    *((size_t*) pool_get(&bytecode)) = BC_RETURN;
    *((size_t*) pool_get(&bytecode)) = 0;
  }

  if (result) {
    size_t bytecode_id = list_append(&ws->bytecode, pool_to_array(&bytecode));
    // printf("Generated bytecode id %zu\n", bytecode_id);
    // inspect_ast_node(node); printf("\n");
    // print_bytecode(list_get(&ws->bytecode, bytecode_id));

    if (node->type == NODE_ASSIGNMENT) {
      AstNode* decl = node->lhs;

      list_append(&ws->initializers, node);
      list_append(&ws->global_scope.declarations, decl);
    }

    node->bytecode_id = bytecode_id;
  }

  return result;
}

bool perform_bytecode_job(Job* job) {
  CompilationWorkspace* ws = job->ws;

  bool result = bytecode_handle_top_level_node(ws, job->node);

  return result;
}

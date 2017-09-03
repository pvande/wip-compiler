void interpreter_begin_dependency_initialization(CompilationWorkspace* ws, AstNode* decl) {
  if (decl->flags & NODE_INITIALIZING) return;

  for (size_t i = 0; i < ws->initializers.length; i++) {
    AstNode* init = list_get(&ws->initializers, i);
    if (init->lhs != decl) continue;

    assert(init->bytecode_id != -1);

    decl->flags |= NODE_INITIALIZING;

    VmState* initializer = malloc(sizeof(VmState));
    initializer->fp = -1;
    initializer->sp = -1;
    initializer->ip = 0;
    initializer->id = init->bytecode_id;
    initializer->waiting_on = NULL;
    pipeline_emit_execute_job(ws, initializer);
  }
}

void interpreter_finish_dependency_initialization(CompilationWorkspace* ws, size_t bytecode_id) {
  for (size_t i = 0; i < ws->initializers.length; i++) {
    AstNode* node = list_get(&ws->initializers, i);
    if (node->bytecode_id != bytecode_id) continue;

    node->lhs->flags &= ~NODE_INITIALIZING;
    node->lhs->flags |= NODE_INITIALIZED;
    break;
  }
}

bool perform_execute_job(Job* job) {
  VmState* state = job->vm_state;
  CompilationWorkspace* ws = job->ws;

  size_t* bytecode = list_get(&ws->bytecode, state->id);

  if (state->waiting_on) {
    if (state->waiting_on->flags & NODE_INITIALIZED) {
      state->waiting_on = NULL;
    } else {
      interpreter_begin_dependency_initialization(ws, state->waiting_on);
      return 0;
    }
  }

  while (1) {
    // inspect_vm_state(state, bytecode);

    switch (bytecode[state->ip++]) {
      case BC_EXIT: {
        if (state->fp == -1) {
          interpreter_finish_dependency_initialization(ws, state->id);
          return 1;
        }

        state->id = state->stack[state->sp--];
        state->ip = state->stack[state->sp--];
        state->fp = state->stack[state->sp--];

        size_t arg_count = state->stack[state->sp--];
        for (size_t i = 0; i < arg_count; i++) state->sp--;

        bytecode = list_get(&job->ws->bytecode, state->id);

        break;
      }

      case BC_LOAD: {
        AstNode* decl = (void*) bytecode[state->ip++];

        state->stack[++state->sp] = (size_t) decl->pointer_value;
        break;
      }

      case BC_STORE: {
        AstNode* decl = (void*) bytecode[state->ip++];

        decl->pointer_value = (void*) state->stack[state->sp--];
        break;
      }

      case BC_LOAD_ARG: {
        size_t offset = bytecode[state->ip++];
        size_t value = state->stack[state->fp - 4 - offset];

        state->stack[++state->sp] = value;
        break;
      }

      case BC_PUSH: {
        size_t value = bytecode[state->ip++];

        state->stack[++state->sp] = value;
        break;
      }

      case BC_CALL: {
        AstNode* decl = (void*) bytecode[state->ip++];

        if (!(decl->flags & NODE_INITIALIZED)) {
          state->ip -= 2;
          state->waiting_on = decl;

          interpreter_begin_dependency_initialization(ws, state->waiting_on);
          return 0;
        }

        AstNode* proc = decl->pointer_value;

        assert(proc != NULL);
        assert(proc->bytecode_id != -1);

        state->stack[++state->sp] = bytecode[state->ip++];  // Number of arguments.
        state->stack[++state->sp] = state->fp;
        state->stack[++state->sp] = state->ip;
        state->stack[++state->sp] = state->id;

        bytecode = list_get(&job->ws->bytecode, proc->bytecode_id);
        state->id = proc->bytecode_id;
        state->fp = state->sp;
        state->ip = 0;

        break;
      }

      case BC_PRINT: {
        putc(state->stack[state->sp--], stdout);
        break;
      }

      default: {
        printf("??? %zu ???\n", bytecode[state->ip - 1]);
        assert(0);
        return 0;
      }
    }
  }

  return 1;
}

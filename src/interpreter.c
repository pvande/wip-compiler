bool perform_execute_job(Job* job) {
  VmState* state = job->vm_state;
  List* initializers = &job->ws->initializers;

  size_t* bytecode = list_get(&job->ws->bytecode, state->id);

  if (state->waiting_on) {
    if (state->waiting_on->flags & NODE_INITIALIZED) {
      state->waiting_on = NULL;
    } else {
      if (!(state->waiting_on->flags & NODE_INITIALIZING)) {
        for (size_t i = 0; i < initializers->length; i++) {
          AstNode* init = list_get(initializers, i);
          if (init->rhs != state->waiting_on) continue;

          assert(init->bytecode_id != -1);

          VmState* init_vm = malloc(sizeof(VmState));
          init_vm->fp = -1;
          init_vm->sp = -1;
          init_vm->ip = 0;
          init_vm->id = init->bytecode_id;
          init_vm->waiting_on = NULL;
          pipeline_emit_execute_job(job->ws, init_vm);

          state->waiting_on->flags |= NODE_INITIALIZING;
        }
      }

      return 0;
    }
  }

  while (1) {
    inspect_vm_state(state, bytecode);

    switch (bytecode[state->ip++]) {
      case BC_EXIT: {
        if (state->fp == -1) {
          for (size_t i = 0; i < initializers->length; i++) {
            AstNode* node = list_get(initializers, i);
            if (node->bytecode_id != state->id) continue;

            node->rhs->flags &= ~NODE_INITIALIZING;
            node->rhs->flags |= NODE_INITIALIZED;
            break;
          }

          return 1;
        }

        // @TODO Return from the current function.

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
      case BC_PUSH: {
        size_t value = bytecode[state->ip++];

        state->stack[++state->sp] = value;
        break;
      }
      case BC_CALL: {
        AstNode* decl = (void*) bytecode[state->ip++];

        if (!(decl->flags & NODE_INITIALIZED)) {
          state->ip -= 2;
          return 0;
        }

        // size_t n = bytecode[state->ip++];
        // for (int i = 0; i < n; i++) state->ip++;
        // exit(1);
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

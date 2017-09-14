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

  #define ARG_COUNT (state->stack[state->fp - 3])
  #define ARG(N)    (state->stack[state->fp - 4 - N])

  while (1) {
    // inspect_vm_state(state, bytecode);

    switch (bytecode[state->ip++]) {
      case BC_EXIT: {
        // fprintf(stderr, "BC_EXIT\n");
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
        // fprintf(stderr, "BC_LOAD %p\n", bytecode[state->ip]);
        AstNode* decl = (void*) bytecode[state->ip++];

        if (!(decl->flags & NODE_INITIALIZED)) {
          state->ip -= BytecodeSizes[BC_LOAD];
          state->waiting_on = decl;

          interpreter_begin_dependency_initialization(ws, state->waiting_on);
          return 0;
        }

        state->stack[++state->sp] = (size_t) decl->pointer_value;
        break;
      }

      case BC_STORE: {
        // fprintf(stderr, "BC_STORE (%p) %p\n", state->stack[state->sp], bytecode[state->ip]);
        AstNode* decl = (void*) bytecode[state->ip++];

        decl->pointer_value = (void*) state->stack[state->sp--];
        decl->flags |= NODE_INITIALIZED;
        break;
      }

      case BC_ARG_LOAD: {
        // fprintf(stderr, "BC_ARG_LOAD %zu\n", bytecode[state->ip]);
        size_t offset = bytecode[state->ip++];

        state->stack[++state->sp] = ARG(offset);
        break;
      }

      case BC_ARG_ADDR: {
        // fprintf(stderr, "BC_ARG_ADDR %zu\n", bytecode[state->ip]);
        size_t offset = bytecode[state->ip++];

        state->stack[++state->sp] = (size_t) &ARG(offset);
        break;
      }

      case BC_PUSH: {
        // fprintf(stderr, "BC_PUSH %zu\n", bytecode[state->ip]);
        size_t value = bytecode[state->ip++];

        state->stack[++state->sp] = value;
        break;
      }

      case BC_CALL: {
        // fprintf(stderr, "BC_CALL %p %zu\n", bytecode[state->ip], bytecode[state->ip + 1]);
        AstNode* decl = (void*) bytecode[state->ip++];
        size_t arg_count = bytecode[state->ip++];

        if (!(decl->flags & NODE_INITIALIZED)) {
          state->ip -= BytecodeSizes[BC_CALL];
          state->waiting_on = decl;

          interpreter_begin_dependency_initialization(ws, state->waiting_on);
          return 0;
        }

        AstNode* proc = decl->pointer_value;

        assert(proc != NULL);
        assert(proc->bytecode_id != -1);

        state->stack[++state->sp] = arg_count;
        state->stack[++state->sp] = state->fp;
        state->stack[++state->sp] = state->ip;
        state->stack[++state->sp] = state->id;

        bytecode = list_get(&job->ws->bytecode, proc->bytecode_id);
        state->id = proc->bytecode_id;
        state->fp = state->sp;
        state->ip = 0;

        break;
      }

      case BC_SYSCALL: {
        // fprintf(stderr, "BC_SYSCALL %zu\n", bytecode[state->ip]);
        size_t args[8] = {};

        size_t arg_count = bytecode[state->ip++];

        for (int i = 0; i < arg_count; i++) {
          args[i] = state->stack[state->sp--];
        }

        if (arg_count == 1) {
          syscall(args[0]);
        } else if (arg_count == 2) {
          syscall(args[0], args[1]);
        } else if (arg_count == 3) {
          syscall(args[0], args[1], args[2]);
        } else if (arg_count == 4) {
          syscall(args[0], args[1], args[2], args[3]);
        } else if (arg_count == 5) {
          syscall(args[0], args[1], args[2], args[3], args[4]);
        } else if (arg_count == 6) {
          syscall(args[0], args[1], args[2], args[3], args[4], args[5]);
        } else if (arg_count == 7) {
          syscall(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        } else if (arg_count == 8) {
          syscall(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        }

        break;
      }

      case BC_JUMP: {
        // fprintf(stderr, "BC_JUMP %lld\n", bytecode[state->ip]);
        size_t distance = bytecode[state->ip++];
        state->ip += distance;
        break;
      }

      case BC_JUMP_ZERO: {
        // fprintf(stderr, "BC_JUMP_ZERO (%zu) %lld\n", state->stack[state->sp], bytecode[state->ip]);
        size_t distance = bytecode[state->ip++];

        size_t test = state->stack[state->sp--];

        if (test == 0) {
          state->ip += distance;
        }

        break;
      }

      case BC_BREAK: {
        assert("Internal Compiler Error: Interpreting BC_BREAK");
      }

      default: {
        printf("??? %zu ???\n", bytecode[state->ip - 1]);
        assert(0);
        return 0;
      }
    }
  }

  #undef ARG_COUNT
  #undef ARG

  return 1;
}

bool perform_execute_job(Job* job) {
  VmState* state = job->vm_state;

  if (state->id == -1) return 0;

  // @Lazy Let's get some real memory in here at some point.
  size_t mem[512];

  // Pool* preload = &job->ws->preload;
  // *((size_t*) pool_get(preload)) = BC_EXIT;
  // print_bytecode(pool_to_array(preload));

  size_t* bytecode = list_get(&job->ws->bytecode, state->id);
  print_bytecode(bytecode); printf("\n");

  while (1) {
    switch (bytecode[state->ip++]) {
      case BC_EXIT: {
        if (state->fp == -1) return 1;

        // @TODO Return from the current function.

        break;
      }
      case BC_LOAD: {
        printf("LOAD %p\n", (void*) bytecode[state->ip]);
        state->stack[++state->sp] = mem[bytecode[state->ip++]];
        break;
      }
      case BC_STORE: {
        printf("STORE %p\n", (void*) bytecode[state->ip]);
        mem[bytecode[state->ip++]] = state->stack[state->sp--];
        break;
      }
      case BC_PUSH: {
        printf("PUSH %zu\n", bytecode[state->ip]);
        state->stack[++state->sp] = bytecode[state->ip++];
        break;
      }
      case BC_CALL: {
        printf("CALL %p\n", (void*) bytecode[state->ip]);
        size_t fn = mem[bytecode[state->ip++]];

        size_t n = bytecode[state->ip++];
        for (int i = 0; i < n; i++) state->ip++;
        exit(1);
        break;
      }
      default : {
        printf("%zu\n", bytecode[state->ip - 1]);
        assert(0);
        return 0;
      }
    }
  }

  return 1;
}

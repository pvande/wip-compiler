typedef struct {
  List list;
  // size_t capacity;
  // size_t bucket_size;
  // size_t size;
  //
  // void*** buckets;

  size_t position;
} Stack;


void initialize_stack(Stack* stack, size_t bucket_count, size_t bucket_size) {
  initialize_list((List*) stack, bucket_count, bucket_size);
  stack->position = 0;
}

Stack* new_stack(size_t bucket_count, size_t bucket_size) {
  assert(bucket_count > 0);
  assert(bucket_size > 0);

  Stack* ret = malloc(sizeof(Stack));
  initialize_stack(ret, bucket_count, bucket_size);
  return ret;
}

size_t stack_length(Stack* stack) {
  return stack->list.length - stack->position;
}

void stack_push(Stack* stack, void* value) {
  list_append((List*) stack, value);
}

void* stack_pop(Stack* stack) {
  void* ptr = list_get((List*) stack, stack->position);
  stack->position -= 1;
  stack->list.length -= 1;

  return ptr;
}

void free_stack(Stack* stack) {
  free_list((List*) stack);
}

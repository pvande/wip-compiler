typedef struct {
  size_t capacity;
  size_t bucket_size;
  size_t length;

  void*** buckets;
} List;


void initialize_list(List* list, size_t bucket_count, size_t bucket_size) {
  list->capacity = bucket_size * bucket_count;
  list->bucket_size = bucket_size;
  list->length = 0;

  if (bucket_size == 0) {
    list->buckets = NULL;
  } else {
    list->buckets = malloc(bucket_count * sizeof(void*));

    for (size_t i = 0; i < bucket_count; i++) {
      list->buckets[i] = malloc(bucket_size * sizeof(void*));
    }
  }
}

List* new_list(size_t bucket_count, size_t bucket_size) {
  assert(bucket_count > 0);

  List* ret = malloc(sizeof(List));
  initialize_list(ret, bucket_count, bucket_size);

  return ret;
}

void* list_get(List* list, size_t idx) {
  if (list->bucket_size == 0) {
    fprintf(stderr, "Internal Compiler Error: Cannot read/write a list with zero-sized buckets!\n");
    exit(1);
  }

  size_t bucket = idx / list->bucket_size;
  size_t bucket_idx = idx % list->bucket_size;

  return list->buckets[bucket][bucket_idx];
}

size_t list_append(List* list, void* value) {
  if (list->bucket_size == 0) {
    fprintf(stderr, "Internal Compiler Error: Cannot read/write a list with zero-sized buckets!\n");
    exit(1);
  }

  if (list->length >= list->capacity) {
    size_t bucket_count = list->capacity / list->bucket_size;
    list->buckets = realloc(list->buckets, (bucket_count + 1) * sizeof(void*));
    list->buckets[bucket_count] = malloc(list->bucket_size * sizeof(void*));
    list->capacity += list->bucket_size;
  }

  size_t bucket = list->length / list->bucket_size;
  size_t bucket_idx = list->length % list->bucket_size;

  list->length += 1;
  list->buckets[bucket][bucket_idx] = value;
  return list->length - 1;
}

void free_list(List* list) {
  if (list->bucket_size > 0) {
    size_t bucket_count = list->capacity / list->bucket_size;

    for (size_t i = 0; i < bucket_count; i++) free(list->buckets[i]);
    free(list->buckets);
  }

  free(list);
}

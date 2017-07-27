typedef struct {
  size_t capacity;
  size_t bucket_size;
  size_t size;

  void*** buckets;
} List;


void initialize_list(List* list, size_t bucket_count, size_t bucket_size) {
  list->capacity = bucket_size * bucket_count;
  list->bucket_size = bucket_size;
  list->size = 0;
  list->buckets = malloc(bucket_count * sizeof(void*));

  for (int i = 0; i < bucket_count; i++) {
    list->buckets[i] = malloc(bucket_size * sizeof(void*));
  }
}

List* new_list(size_t bucket_count, size_t bucket_size) {
  assert(bucket_count > 0);
  assert(bucket_size > 0);

  List* ret = malloc(sizeof(List));
  initialize_list(ret, bucket_count, bucket_size);

  return ret;
}

void* list_get(List* list, size_t idx) {
  size_t bucket = idx / list->bucket_size;
  size_t bucket_idx = idx % list->bucket_size;

  return list->buckets[bucket][bucket_idx];
}

size_t list_add(List* list, void* value) {
  if (list->size >= list->capacity) {
    size_t bucket_count = list->capacity / list->bucket_size;
    list->buckets = realloc(list->buckets, (bucket_count + 1) * sizeof(void*));
    list->buckets[bucket_count] = malloc(list->bucket_size * sizeof(void*));
    list->capacity += list->bucket_size;
  }

  size_t bucket = list->size / list->bucket_size;
  size_t bucket_idx = list->size % list->bucket_size;

  list->size += 1;
  list->buckets[bucket][bucket_idx] = value;
  return list->size;
}

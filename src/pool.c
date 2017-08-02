typedef struct {
  size_t capacity;
  size_t bucket_size;
  size_t slot_size;
  size_t length;

  void*** buckets;
} Pool;

void initialize_pool(Pool* pool, size_t slot_size, size_t bucket_count, size_t bucket_size) {
  pool->capacity = bucket_size * bucket_count;
  pool->bucket_size = bucket_size;
  pool->slot_size = slot_size;
  pool->length = 0;

  pool->buckets = malloc(bucket_count * sizeof(void*));

  for (size_t i = 0; i < bucket_count; i++) {
    pool->buckets[i] = malloc(bucket_size * slot_size);
  }
}

Pool* new_pool(size_t slot_size, size_t bucket_count, size_t bucket_size) {
  assert(slot_size > 0);
  assert(bucket_count > 0);
  assert(bucket_size > 0);

  Pool* ret = malloc(sizeof(Pool));
  initialize_pool(ret, slot_size, bucket_count, bucket_size);

  return ret;
}

void* pool_get(Pool* pool) {
  if (pool->length >= pool->capacity) {
    size_t bucket_count = pool->capacity / pool->bucket_size;
    pool->buckets = realloc(pool->buckets, (bucket_count + 1) * sizeof(void*));
    pool->buckets[bucket_count] = malloc(pool->bucket_size * pool->slot_size);
    pool->capacity += pool->bucket_size;
  }

  size_t bucket = pool->length / pool->bucket_size;
  size_t bucket_idx = pool->length % pool->bucket_size;

  pool->length += 1;
  return (void*) (((size_t) pool->buckets[bucket]) + (bucket_idx * pool->slot_size));
}

void free_pool(Pool* pool) {
  size_t bucket_count = pool->capacity / pool->bucket_size;

  for (size_t i = 0; i < bucket_count; i++) {
    free(pool->buckets[i]);
  }
  free(pool->buckets);
  free(pool);
}

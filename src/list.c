typedef struct {
  size_t capacity;
  size_t bucket_size;
  size_t size;

  void*** buckets;
} List;

List* new_list(size_t bucket_count, size_t bucket_size) {
  assert(bucket_count > 0);
  assert(bucket_size > 0);

  List* ret = malloc(sizeof(List));
  ret->capacity = bucket_size * bucket_count;
  ret->bucket_size = bucket_size;
  ret->size = 0;
  ret->buckets = malloc(bucket_count * sizeof(void*));

  for (int i = 0; i < bucket_count; i++) {
    ret->buckets[i] = malloc(bucket_size * sizeof(void*));
  }

  return ret;
}

void* list_get(List* list, size_t idx) {
  size_t bucket = idx / list->bucket_size;
  size_t bucket_idx = idx % list->bucket_size;

  return list->buckets[bucket][bucket_idx];
}

size_t list_add(List* list, void* value) {
  list->size += 1;
  if (list->size > list->capacity) {
    size_t bucket_count = list->capacity / list->bucket_size;
    list->buckets = realloc(list->buckets, (bucket_count + 1) * sizeof(void*));
    list->capacity += list->bucket_size;
  }

  size_t bucket = list->size / list->bucket_size;
  size_t bucket_idx = list->size % list->bucket_size;

  list->buckets[bucket][bucket_idx] = value;
  return list->size;
}

// void table_resize(Table* t, size_t size) {
//   Table* tmp = new_table(size);
//
//   for (int i = 0; i < t->capacity; i++) {
//     if (! t->occupied[i]) continue;
//     __table_store(tmp, t->keys[i], t->values[i]);
//   }
//
//   free(t->occupied);
//   free(t->keys);
//   free(t->values);
//
//   t->capacity = tmp->capacity;
//   t->occupied = tmp->occupied;
//   t->keys = tmp->keys;
//   t->values = tmp->values;
// }
//
// void table_add(Table* t, String* key, void* value) {
//   if (t->size >= t->capacity) {
//     table_resize(t, t->capacity * 1.5);
//   }
//
//   __table_store(t, key, value);
// }
//
// void* table_find(Table* t, String* key) {
//   size_t slot = __table_find_slot_for_key(t, key);
//   if (slot == -1 || !t->occupied[slot]) return NULL;
//
//   return t->values[slot];
// }
//
// void free_table(Table* t) {
//   free(t->occupied);
//   free(t->keys);
//   free(t->values);
//   free(t);
// }

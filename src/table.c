uint32_t __hash__ (String* str) {
  size_t len = str->length;
  char* data = str->data;
  size_t hash = len;
  size_t tmp;
  char rem;

  if (len <= 0 || data == NULL) return 0;

  rem = len & 3;
  len >>= 2;

  for (;len > 0; len--) {
      hash  += *((const uint16_t*) data) ;
      tmp    = (*((const uint16_t*) (data+2)) << 11) ^ hash;
      hash   = (hash << 16) ^ tmp;
      data  += 2*sizeof (uint16_t);
      hash  += hash >> 11;
  }

  switch (rem) {
      case 3: hash += *((const uint16_t*) data);
              hash ^= hash << 16;
              hash ^= ((signed char) data[sizeof (uint16_t)]) << 18;
              hash += hash >> 11;
              break;
      case 2: hash += *((const uint16_t*) data);
              hash ^= hash << 11;
              hash += hash >> 17;
              break;
      case 1: hash += (signed char)*data;
              hash ^= hash << 10;
              hash += hash >> 1;
  }

  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;
}

typedef struct {
  size_t capacity;
  size_t size;

  char* occupied;
  String** keys;
  void** values;
} Table;

Table* new_table(size_t capacity) {
  assert(capacity != 0);

  Table* ret = malloc(sizeof(Table));
  ret->capacity = capacity;
  ret->size = 0;
  ret->occupied = calloc(capacity, sizeof(char));
  ret->keys = malloc(capacity * sizeof(String*));
  ret->values = malloc(capacity * sizeof(void*));

  return ret;
}

size_t __table_find_slot_for_key(Table* t, String* key) {
  size_t slot = __hash__(key);
  size_t steps = 0;

  do {
    slot += 1;
    slot %= t->capacity;
    if (!t->occupied[slot] || string_equals(key, t->keys[slot])) return slot;
    steps += 1;
  } while(t->occupied[slot] && steps < t->capacity);

  return -1;
}

void __table_store(Table* t, String* key, void* value) {
  size_t slot = __table_find_slot_for_key(t, key);
  assert(slot != -1);

  if (!t->occupied[slot]) t->size += 1;

  t->occupied[slot] = 1;
  t->keys[slot] = key;
  t->values[slot] = value;
}

void table_resize(Table* t, size_t size) {
  Table* tmp = new_table(size);

  for (int i = 0; i < t->capacity; i++) {
    if (! t->occupied[i]) continue;
    __table_store(tmp, t->keys[i], t->values[i]);
  }

  free(t->occupied);
  free(t->keys);
  free(t->values);

  t->capacity = tmp->capacity;
  t->occupied = tmp->occupied;
  t->keys = tmp->keys;
  t->values = tmp->values;
}

void table_add(Table* t, String* key, void* value) {
  if (t->size >= t->capacity) {
    table_resize(t, t->capacity * 1.5);
  }

  __table_store(t, key, value);
}

void* table_find(Table* t, String* key) {
  size_t slot = __table_find_slot_for_key(t, key);
  if (slot == -1 || !t->occupied[slot]) return NULL;

  return t->values[slot];
}

void free_table(Table* t) {
  free(t->occupied);
  free(t->keys);
  free(t->values);
  free(t);
}

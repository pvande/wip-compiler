void test_pool_creation() {
  Pool* pool;

  TEST("Creating a new pool(1, 4, 8)");
  pool = new_pool(1, 4, 8);
  ASSERT_EQ(pool->length, 0, "has a length of zero");
  ASSERT_EQ(pool->slot_size, 1, "has a slot size of 1");
  ASSERT_EQ(pool->bucket_size, 8, "has a bucket size of 8");
  ASSERT_EQ(pool->capacity, 32, "has 32 slots capacity");
  free_pool(pool);
}

void test_pool_get() {
  Pool* pool;

  TEST("Getting items from a pool(1, 1, 1)");
  pool = new_pool(1, 1, 1);
  ASSERT_EQ(pool->length, 0, "has a length of zero");
  ASSERT_EQ(pool->slot_size, 1, "has a slot size of 1");
  ASSERT_EQ(pool->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(pool->capacity, 1, "has 1 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 1, "has a length of 1");
  ASSERT_EQ(pool->slot_size, 1, "has a slot size of 1");
  ASSERT_EQ(pool->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(pool->capacity, 1, "has 1 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 2, "has a length of 2");
  ASSERT_EQ(pool->slot_size, 1, "has a slot size of 1");
  ASSERT_EQ(pool->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(pool->capacity, 2, "has 2 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 3, "has a length of 3");
  ASSERT_EQ(pool->slot_size, 1, "has a slot size of 1");
  ASSERT_EQ(pool->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(pool->capacity, 3, "has 3 slots capacity");
  free_pool(pool);

  TEST("Getting items from a pool(sizeof(size_t), 1, 4)");
  pool = new_pool(sizeof(size_t), 1, 4);
  ASSERT_EQ(pool->length, 0, "has a length of zero");
  ASSERT_EQ(pool->slot_size, sizeof(size_t), "has a slot size of sizeof(size_t)");
  ASSERT_EQ(pool->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(pool->capacity, 4, "has 4 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 1, "has a length of 1");
  ASSERT_EQ(pool->slot_size, sizeof(size_t), "has a slot size of sizeof(size_t)");
  ASSERT_EQ(pool->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(pool->capacity, 4, "has 4 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 2, "has a length of 2");
  ASSERT_EQ(pool->slot_size, sizeof(size_t), "has a slot size of sizeof(size_t)");
  ASSERT_EQ(pool->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(pool->capacity, 4, "has 4 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 3, "has a length of 3");
  ASSERT_EQ(pool->slot_size, sizeof(size_t), "has a slot size of sizeof(size_t)");
  ASSERT_EQ(pool->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(pool->capacity, 4, "has 4 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 4, "has a length of 4");
  ASSERT_EQ(pool->slot_size, sizeof(size_t), "has a slot size of sizeof(size_t)");
  ASSERT_EQ(pool->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(pool->capacity, 4, "has 4 slots capacity");
  pool_get(pool);
  ASSERT_EQ(pool->length, 5, "has a length of 5");
  ASSERT_EQ(pool->slot_size, sizeof(size_t), "has a slot size of sizeof(size_t)");
  ASSERT_EQ(pool->bucket_size, 4, "has a bucket size of 5");
  ASSERT_EQ(pool->capacity, 8, "has 5 slots capacity");
  free_pool(pool);

  TEST("Setting values in a pool");
  pool = new_pool(sizeof(String), 1, 3);
  String* x = pool_get(pool);

  String* y = x + 1;
  y->data = "hey";
  y->length = 3;
  String* expected = pool_get(pool);

  String* z = pool_get(pool);
  z->data = "nope";
  z->length = 4;

  ASSERT_EQ(expected, y, "correctly returns subsequent slots");
  ASSERT_STR_EQ(expected, new_string("hey"), "correctly allocates adjacent slots");
}

void test_pool_to_array() {
  Pool* pool;
  char* c;

  pool = new_pool(sizeof(String), 1, 1);
  *((String*) pool_get(pool)) = *new_string("First!");
  *((String*) pool_get(pool)) = *new_string("Second!");
  *((String*) pool_get(pool)) = *new_string("Last!");

  TEST("Converting a pool of structs yields an equivalent array of structs");
  String* str = pool_to_array(pool);
  ASSERT_STR_EQ(&str[0], new_string("First!"), "correctly returns the assigned struct");
  ASSERT_STR_EQ(&str[1], new_string("Second!"), "correctly returns the assigned struct");
  ASSERT_STR_EQ(&str[2], new_string("Last!"), "correctly returns the assigned struct");
  free_pool(pool);
  free(str);

  // Initially only stores one byte; buckets will be at random memory locations.
  pool = new_pool(1, 1, 1);
  *((char*) pool_get(pool)) = 'h';
  *((char*) pool_get(pool)) = 'e';
  *((char*) pool_get(pool)) = 'l';
  void* ptr = malloc(100);  // Just to ensure the allocations *cannot* be adjacent.
  *((char*) pool_get(pool)) = 'l';
  *((char*) pool_get(pool)) = 'o';
  *((char*) pool_get(pool)) = '!';
  *((char*) pool_get(pool)) = '\0';
  free(ptr);

  TEST("Converting a multi-byte pool to an array of bytes");
  c = pool_to_array(pool);
  ASSERT_EQ(strcmp(c, "hello!"), 0, "correctly returns a byte sequence as a unified array");
  free_pool(pool);
  ASSERT_EQ(strcmp(c, "hello!"), 0, "copies values into the new array");
  free(c);

  // Initially stores only three bytes in one bucket.
  pool = new_pool(1, 1, 3);
  *((char*) pool_get(pool)) = 'h';  // Bucket 0
  *((char*) pool_get(pool)) = 'e';  // Bucket 0
  *((char*) pool_get(pool)) = 'l';  // Bucket 0
  *((char*) pool_get(pool)) = 'l';  // Bucket 1
  *((char*) pool_get(pool)) = 'o';  // Bucket 1
  *((char*) pool_get(pool)) = '!';  // Bucket 1
  c = pool_get(pool);  // Creates Bucket 2
  *(c + 0) = '\0';
  *(c + 1) = '\xFF';
  *(c + 2) = '\xFF';

  // Pool now has a length of 7, but has 9 bucket slots allocated.

  TEST("Converting a multi-byte pool to an array of bytes omits unoccupied bucket slots");
  c = pool_to_array(pool);
  ASSERT_EQ(strcmp(c, "hello!"), 0, "correctly returns a byte sequence as a unified array");
  ASSERT_NOT_EQ(c[7], '\xFF', "does not copy slot 7 into the new array");
  ASSERT_NOT_EQ(c[8], '\xFF', "does not copy slot 8 into the new array");
  free_pool(pool);
  free(c);

  // Initially stores three bytes in three different buckets.
  pool = new_pool(1, 3, 1);
  pool->length = 1;
  *((char*) pool->buckets[0]) = '#';
  *((char*) pool->buckets[1]) = '\xFF';
  *((char*) pool->buckets[2]) = '\xFF';

  // Pool now has a length of 7, but has 9 bucket slots allocated.

  TEST("Converting a multi-byte pool to an array of bytes omits unoccupied buckets");
  c = pool_to_array(pool);
  ASSERT_EQ(c[0], '#', "correctly returns the relevant bytes");
  ASSERT_NOT_EQ(c[1], '\xFF', "does not copy slot 1 into the new array");
  ASSERT_NOT_EQ(c[2], '\xFF', "does not copy slot 2 into the new array");
  free_pool(pool);
  free(c);
}


void run_all_pool_tests() {
  test_pool_creation();
  test_pool_get();
  test_pool_to_array();
}

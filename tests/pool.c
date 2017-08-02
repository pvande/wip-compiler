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


void run_all_pool_tests() {
  test_pool_creation();
  test_pool_get();
}

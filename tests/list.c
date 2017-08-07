void test_list_creation() {
  List* list;

  TEST("Creating a new list(4, 8)");
  list = new_list(4, 8);
  ASSERT_EQ(list->length, 0, "has a length of zero");
  ASSERT_EQ(list->bucket_size, 8, "has a bucket size of 8");
  ASSERT_EQ(list->capacity, 32, "has 32 slots capacity");
  free_list(list);
}

void test_list_append() {
  List* list;

  TEST("Appending items to a list(1, 1)");
  list = new_list(1, 1);
  ASSERT_EQ(list->length, 0, "has a length of zero");
  ASSERT_EQ(list->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(list->capacity, 1, "has 1 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 1, "has a length of 1");
  ASSERT_EQ(list->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(list->capacity, 1, "has 1 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 2, "has a length of 2");
  ASSERT_EQ(list->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(list->capacity, 2, "has 2 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 3, "has a length of 3");
  ASSERT_EQ(list->bucket_size, 1, "has a bucket size of 1");
  ASSERT_EQ(list->capacity, 3, "has 3 slots capacity");
  free_list(list);

  TEST("Appending items to a list(1, 4)");
  list = new_list(1, 4);
  ASSERT_EQ(list->length, 0, "has a length of zero");
  ASSERT_EQ(list->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(list->capacity, 4, "has 4 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 1, "has a length of 1");
  ASSERT_EQ(list->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(list->capacity, 4, "has 4 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 2, "has a length of 2");
  ASSERT_EQ(list->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(list->capacity, 4, "has 4 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 3, "has a length of 3");
  ASSERT_EQ(list->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(list->capacity, 4, "has 4 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 4, "has a length of 4");
  ASSERT_EQ(list->bucket_size, 4, "has a bucket size of 4");
  ASSERT_EQ(list->capacity, 4, "has 4 slots capacity");
  list_append(list, NULL);
  ASSERT_EQ(list->length, 5, "has a length of 5");
  ASSERT_EQ(list->bucket_size, 4, "has a bucket size of 5");
  ASSERT_EQ(list->capacity, 8, "has 5 slots capacity");
  free_list(list);
}

void test_list_get() {
  List* list;

  list = new_list(1, 4);
  list_append(list, NULL);
  list_append(list, NULL);
  list_append(list, list);
  list_append(list, NULL);

  TEST("Finding the existing keys");
  ASSERT_EQ(list_get(list, 0), NULL, "returns the value added first");
  ASSERT_EQ(list_get(list, 1), NULL, "returns the value added second");
  ASSERT_EQ(list_get(list, 2), list, "returns the value added third");
  ASSERT_EQ(list_get(list, 3), NULL, "returns the value added fourth");

  free_list(list);
}

void run_all_list_tests() {
  test_list_creation();
  test_list_append();
  test_list_get();
}

void test_table_creation() {
  Table* t;

  TEST("Creating a new table(4)");
  t = new_table(4);
  ASSERT_EQ(t->size, 0, "has a size of zero");
  ASSERT_EQ(t->capacity, 4, "has 4 slots capacity");
  free_table(t);

  // TEST("Creating a new table(0)");
  // t = new_table(0);
  // ASSERT_EQ(t->size, 0, "has a size of zero");
  // ASSERT_EQ(t->capacity, 0, "has 0 slots capacity");
  // free_table(t);
}

void test_table_add() {
  Table* t;

  TEST("Adding one item to a table(2)");
  t = new_table(2);
  table_add(t, new_string("A"), new_string("A Value"));
  ASSERT_EQ(t->size, 1, "has a size of one");
  ASSERT_EQ(t->capacity, 2, "has 2 slots capacity");
  free_table(t);

  TEST("Adding two items to a table(2)");
  t = new_table(2);
  table_add(t, new_string("A"), new_string("A Value"));
  table_add(t, new_string("B"), new_string("B Value"));
  ASSERT_EQ(t->size, 2, "has a size of two");
  ASSERT_EQ(t->capacity, 2, "has 2 slots capacity");
  free_table(t);

  TEST("Adding three items to a table(2)");
  t = new_table(2);
  table_add(t, new_string("A"), new_string("A Value"));
  table_add(t, new_string("B"), new_string("B Value"));
  table_add(t, new_string("C"), new_string("C Value"));
  ASSERT_EQ(t->size, 3, "has a size of three");
  ASSERT_EQ(t->capacity, 3, "has 3 slots capacity");
  free_table(t);

  TEST("Adding four items to a table(2)");
  t = new_table(2);
  table_add(t, new_string("A"), new_string("A Value"));
  table_add(t, new_string("B"), new_string("B Value"));
  table_add(t, new_string("C"), new_string("C Value"));
  table_add(t, new_string("D"), new_string("D Value"));
  ASSERT_EQ(t->size, 4, "has a size of four");
  ASSERT_EQ(t->capacity, 4, "has 4 slots capacity");
  free_table(t);

  TEST("Adding five items to a table(2)");
  t = new_table(2);
  table_add(t, new_string("A"), new_string("A Value"));
  table_add(t, new_string("B"), new_string("B Value"));
  table_add(t, new_string("C"), new_string("C Value"));
  table_add(t, new_string("D"), new_string("D Value"));
  table_add(t, new_string("E"), new_string("E Value"));
  ASSERT_EQ(t->size, 5, "has a size of five");
  ASSERT_EQ(t->capacity, 6, "has 6 slots capacity");
  free_table(t);

  TEST("Adding a duplicate item to a table(4)");
  t = new_table(2);
  table_add(t, new_string("A"), new_string("A Value"));
  table_add(t, new_string("A"), new_string("a Value"));
  ASSERT_EQ(t->size, 1, "has a size of one");
  ASSERT_EQ(t->capacity, 2, "has 2 slots capacity");
  free_table(t);
}

void test_table_find() {
  Table* t;

  t = new_table(5);

  TEST("Finding a key that doesn't exist");
  ASSERT_EQ(table_find(t, new_string("D")), NULL, "returns NULL");
  ASSERT_EQ(table_find(t, new_string("C")), NULL, "returns NULL");
  ASSERT_EQ(table_find(t, new_string("B")), NULL, "returns NULL");
  ASSERT_EQ(table_find(t, new_string("A")), NULL, "returns NULL");

  table_add(t, new_string("A"), new_string("A Value"));
  table_add(t, new_string("B"), new_string("B Value"));
  table_add(t, new_string("C"), new_string("C Value"));
  table_add(t, new_string("D"), new_string("D Value"));

  TEST("Finding the existing keys");
  ASSERT_STR_EQ(table_find(t, new_string("D")), new_string("D Value"), "returns the associated D value");
  ASSERT_STR_EQ(table_find(t, new_string("C")), new_string("C Value"), "returns the associated C value");
  ASSERT_STR_EQ(table_find(t, new_string("B")), new_string("B Value"), "returns the associated B value");
  ASSERT_STR_EQ(table_find(t, new_string("A")), new_string("A Value"), "returns the associated A value");
  free_table(t);
}

void run_all_table_tests() {
  test_table_creation();
  test_table_add();
  test_table_find();
}

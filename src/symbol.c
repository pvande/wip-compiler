typedef size_t Symbol;

Table* __symbol_table = NULL;
List* __symbol_lookup = NULL;

void _initialize_symbol_data() {
  __symbol_table = new_table(256);
  __symbol_lookup = new_list(8, 32);
}

Symbol symbol_get(String* text) {
  // @TODO Eagerly initialize these.
  if (__symbol_table == NULL) _initialize_symbol_data();

  Symbol id = (Symbol) table_find(__symbol_table, text);
  if ((void*) id == NULL) {
    String* copy = malloc(sizeof(String));
    memcpy(copy, text, sizeof(String));

    id = list_append(__symbol_lookup, copy);
    table_add(__symbol_table, copy, (void*) id);
  }

  return id;
}

String* symbol_lookup(Symbol id) {
  // @TODO Eagerly initialize these.
  if (__symbol_table == NULL) _initialize_symbol_data();

  return list_get(__symbol_lookup, id - 1);
}

void* type_create_untracked(String* name, size_t size) {
  static size_t serial = 0;  // @TODO Don't use static...

  Typeclass* type = calloc(1, sizeof(Typeclass));
  type->id = serial++;
  type->size = size;
  type->name = name;

  return type;
}

void type_alias(CompilationWorkspace* ws, String* name, Typeclass* type) {
  table_add(&ws->typeclasses, name, type);
}

void* type_create(CompilationWorkspace* ws, String* name, size_t size) {
  Typeclass* type = type_create_untracked(name, size);
  type_alias(ws, name, type);
  return type;
}

void* type_find(CompilationWorkspace* ws, String* name) {
  return table_find(&ws->typeclasses, name);
}

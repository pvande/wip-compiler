typedef struct {
  size_t length;
  char* data;
} String;


void initialize_string(String* string, char* data) {
  string->length = strlen(data);
  string->data = data;
}

String* new_string(char* data) {
  String* str = malloc(sizeof(String));
  initialize_string(str, data);
  return str;
}

char* to_zero_terminated_string(String* str) {
  char* buffer = calloc(str->length + 1, sizeof(char));

  memcpy(buffer, str->data, str->length);

  return buffer;
}

String substring(String* str, size_t pos, size_t length) {
  String substr = { length, str->data + pos };
  return substr;
}

char string_equals(String* a, String* b) {
  if (a->length != b->length) return 0;

  for (int i = 0; i < a->length; i++) {
    if (a->data[i] != b->data[i]) return 0;
  }

  return 1;
}

typedef struct {
  size_t length;
  char* data;
} String;


String* new_string(char* s) {
  String* str = malloc(sizeof(String));

  str->length = strlen(s);
  str->data = s;

  return str;
}

char* to_zero_terminated_string(String* str) {
  char* buffer = calloc(str->length, sizeof(char));

  memcpy(buffer, str->data, str->length);
  buffer[str->length] = '\0';

  return buffer;
}

String* substring(String* str, size_t pos, size_t length) {
  String* substr = malloc(sizeof(String));

  substr->length = length;
  substr->data = str->data + pos;

  return substr;
}

char string_equals(String* a, String* b) {
  if (a->length != b->length) return 0;

  for (int i = 0; i < a->length; i++) {
    if (a->data[i] != b->data[i]) return 0;
  }

  return 1;
}

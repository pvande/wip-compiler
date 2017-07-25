typedef struct {
  size_t length;
  char* data;
} String;

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

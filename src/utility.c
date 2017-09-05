String* unescape_string_literal(String* source) {
  String* str = malloc(sizeof(String) + source->length * sizeof(char));
  str->data = (char*) (str + 1);

  assert(source->data[0] == '"');
  assert(source->data[source->length - 1] == '"');

  size_t idx = 0;
  for (size_t i = 0; i < source->length - 2; i++) {
    char c = source->data[i + 1];

    if (c == '\\') {
      i += 1;
      switch (source->data[i + 1]) {
        case '0': str->data[idx++] = '\0'; break;
        case 'a': str->data[idx++] = '\a'; break;
        case 'b': str->data[idx++] = '\b'; break;
        case 't': str->data[idx++] = '\t'; break;
        case 'n': str->data[idx++] = '\n'; break;
        case 'v': str->data[idx++] = '\v'; break;
        case 'f': str->data[idx++] = '\f'; break;
        case 'r': str->data[idx++] = '\r'; break;
        case 'e': str->data[idx++] = '\e'; break;
        case '"': str->data[idx++] = '\"'; break;
        case '\\': str->data[idx++] = '\\'; break;
        default: {
          char* err = malloc(32 * sizeof(char));
          sprintf(err, "Unknown escape sequence: '\\%c'\n", source->data[i + 1]);

          // node->flags |= NODE_CONTAINS_ERROR;
          // node->error = new_string(err);
          // return 1;
        }
      }
    } else {
      str->data[idx++] = c;
    }
  }
  str->length = idx;

  str = realloc(str, sizeof(String) + idx * sizeof(char));
  str->data = (char*) (str + 1);

  return str;
}

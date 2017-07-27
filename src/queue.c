typedef struct {
  List list;
  // size_t capacity;
  // size_t bucket_size;
  // size_t size;
  //
  // void*** buckets;

  size_t position;
} Queue;


void initialize_queue(Queue* queue, size_t bucket_count, size_t bucket_size) {
  initialize_list((List*) queue, bucket_count, bucket_size);
  queue->position = 0;
}

Queue* new_queue(size_t bucket_count, size_t bucket_size) {
  assert(bucket_count > 0);
  assert(bucket_size > 0);

  Queue* ret = malloc(sizeof(Queue));
  initialize_queue(ret, bucket_count, bucket_size);
  return ret;
}

size_t queue_length(Queue* queue) {
  return queue->list.size - queue->position;
}

void queue_add(Queue* queue, void* value) {
  list_add((List*) queue, value);
}

void* queue_pull(Queue* queue) {
  void* ptr = list_get((List*) queue, queue->position);
  queue->position += 1;

  // If we've emptied a bucket we should rotate them, so that we can avoid
  // unnecessary allocations.
  // @TODO There may be a better way of handling this.
  if (queue->position == queue->list.bucket_size) {
    queue->position = 0;
    void** empty = queue->list.buckets[0];

    size_t bucket_count = queue->list.capacity / queue->list.bucket_size;
    for (int i = 0; i < bucket_count - 1; i++) {
      queue->list.buckets[i] = queue->list.buckets[i + 1];
    }
    queue->list.buckets[bucket_count - 1] = empty;
  }

  return ptr;
}

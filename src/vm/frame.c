// Placing all frame code here for now and later refactor some into frame.h
#include "frame.h"
#include "threads/palloc.h"
#include <hash.h>

struct frame_table_entry {
  int frame_no;
  void *page;
  int64_t ticks;
  struct hash_elem helem;
};

struct hash frame_table;

unsigned frame_hash_func(const struct hash_elem *element, void *aux)
{
  return hash_int(hash_entry(element, struct frame_table_entry, helem)->frame_no);
}

bool frame_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
  struct frame_table_entry *entrya = hash_entry(a, struct frame_table_entry, helem);
  struct frame_table_entry *entryb = hash_entry(b, struct frame_table_entry, helem);

  return entrya->frame_no < entryb->frame_no;
}

void frame_init(void)
{
  hash_init(&frame_table, &frame_hash_func, &frame_less_func, NULL);
}
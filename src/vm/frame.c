// Placing all frame code here for now and later refactor some into frame.h
#include "frame.h"
#include "threads/palloc.h"
#include <hash.h>

struct frame_table_entry {
  int frame_no;
  void *page;
  int64_t ticks;
  struct list_elem elem;
};

struct list frame_table;

unsigned frame_hash_func(const struct list_elem *element, void *aux)
{
  return hash_int(list_entry(element, struct frame_table_entry, elem)->frame_no);
}

bool frame_less_func(const struct list_elem *a, const struct list_elem *b, void *aux)
{
  struct frame_table_entry *entrya = list_entry(a, struct frame_table_entry, elem);
  struct frame_table_entry *entryb = list_entry(b, struct frame_table_entry, elem);

  return entrya->frame_no < entryb->frame_no;
}

void frame_init(void)
{
  list_init(&frame_table);
}
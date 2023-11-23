// Placing all frame code here for now and later refactor some into frame.h
#include "frame.h"
#include "threads/palloc.h"
#include <hash.h>
#include "lib/debug.h"
#include "timer.h"

#define FRAME_BITS 20
#define MAX_FRAME_NUMBER (1 << FRAME_BITS)

struct frame_table_entry {
  int frame_no;
  void *page;
  int64_t ticks;
  struct hash_elem helem;
};

struct hash frame_table;

unsigned frame_hash_func(const struct hash_elem *element, void *aux UNUSED)
{
  return hash_int(hash_entry(element, struct frame_table_entry, helem)->frame_no);
}

bool frame_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct frame_table_entry *entrya = hash_entry(a, struct frame_table_entry, helem);
  struct frame_table_entry *entryb = hash_entry(b, struct frame_table_entry, helem);

  return entrya->frame_no < entryb->frame_no;
}

void frame_init(void)
{
  bool success = hash_init(&frame_table, frame_hash_func, frame_less_func, NULL);
  if (!success) PANIC("Frame table could not be allocated");
}

// Returns the frame_table_entry * with the corresponding frame_no
struct frame_table_entry *frame_lookup(int frame_no)
{
  struct frame_table_entry f;
  f.frame_no = frame_no;
  struct hash_elem *e = hash_find(&frame_table, &f.helem);
  return e != NULL ? hash_entry(e, struct frame_table_entry, helem) : NULL;
}

void frame_add(int frame_no, void *page)
{
  struct frame_table_entry *frame = malloc(sizeof(struct frame_table_entry));
  if (frame == NULL) PANIC("Frame entry could not be initialised");

  *frame = (struct frame_table_entry) {
    .frame_no = frame_no,
    .page = page,
    .ticks = timer_ticks()
  };
  hash_insert(&frame_table, &frame->helem);
}

// Deletes given frame_no from frame_table and frees it
void frame_remove(int frame_no)
{
  struct frame_table_entry f;
  f.frame_no = frame_no;
  struct hash_elem *e = hash_delete(&frame_table, &f.helem);

  if (e != NULL)
  {
    free(hash_entry(e, struct frame_table_entry, helem))
  }
}

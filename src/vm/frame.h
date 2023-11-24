#include <hash.h>
#include "lib/user/syscall.h"

struct frame_table_entry {
  int frame_no;
  pid_t pid;
  void *page;
  int64_t ticks;
  struct hash_elem helem;
};

extern unsigned frame_hash_func(const struct hash_elem *element, void *aux);
extern bool frame_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);
extern void frame_free_func(struct hash_elem *e, void *aux);

extern void frame_init(void);
extern struct frame_table_entry *frame_lookup(int frame_no);
extern void frame_add(pid_t pid);
extern void frame_remove(int frame_no);
extern void frame_remove_process(pid_t pid);
extern void frame_clear(void);
extern void frame_destroy(void);

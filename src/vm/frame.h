#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "devices/swap.h"

struct frame {
  void *paddr;
  struct spt_entry *spte;
  struct thread *thread;
  struct hash_elem helem;
};

void frame_table_init(void);
void add_frame(struct frame* frame);
void delete_frame(struct frame *frame);

void evict_frames(void);
struct frame *allocate_frame(enum palloc_flags alloc_flag);
void free_frame(void *paddr);
void free_frame_helper(struct frame *frame);

#endif

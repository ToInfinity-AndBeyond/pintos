#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "devices/swap.h"

struct frame {
  void *paddr;                  /* Physical address */
  struct spt_entry *spte;       /* Supplemental page table entry */
  struct thread *thread;        /* Owning thread */
  struct list_elem clock_elem;  /* Allows insertion into frame table */
};

struct lock clock_list_lock;
struct lock eviction_lock;
struct list clock_list;
struct list_elem *clock_elem;

void frame_table_init(void);
void add_frame(struct frame* frame);
void delete_frame(struct frame *frame);

void evict_frames(void);
struct frame *share_existing_page(struct spt_entry *spte); 
struct frame *allocate_frame(enum palloc_flags alloc_flag);
void free_frame(void *paddr);
void free_frame_helper(struct frame *frame);

#endif

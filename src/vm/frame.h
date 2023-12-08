#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "devices/swap.h"


struct list clock_list;
struct lock clock_list_lock;
struct lock eviction_lock;
struct list_elem *clock_elem;

void frame_locks_init(void);
void delete_page(struct page *page);

struct page *share_existing_page(struct spt_entry *spte);
struct page *allocate_page(enum palloc_flags alloc_flag);
void free_page(void *paddr);

#endif

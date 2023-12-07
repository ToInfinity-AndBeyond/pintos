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
struct list_elem *clock_elem;

void clock_list_init(void);
void add_page(struct page* page);
void delete_page(struct page *page);

void evict_pages(void);
struct page *allocate_page(enum palloc_flags alloc_flag);
void free_page(void *paddr);
void free_page_helper(struct page *page);

#endif
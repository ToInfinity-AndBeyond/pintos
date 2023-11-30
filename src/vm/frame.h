#include <hash.h>
#include <list.h>
#include "threads/synch.h"
#include "vm/page.h"
#include "threads/palloc.h"

struct list clock_list;
struct lock clock_list_lock;
struct list_elem *clock_elem;

void clock_list_init(void);
void add_page_to_clock_list(struct page* page);
void del_page_from_clock_list(struct page *page);

void try_to_free_pages(enum palloc_flags alloc_flag);
struct page *alloc_page(enum palloc_flags alloc_flag);
void free_page(void *paddr);
void _free_page(struct page *page);

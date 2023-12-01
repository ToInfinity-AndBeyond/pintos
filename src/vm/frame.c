#include "frame.h"

static struct list_elem* find_next_clock()
{
    if(list_empty(&clock_list))
        return NULL;

    if(clock_elem == NULL || clock_elem == list_end(&clock_list))
    {
        return list_begin(&clock_list);
    }

    if (list_next(clock_elem) == list_end(&clock_list))
    {
        return list_begin(&clock_list);
    }
    else
    {
        return list_next(clock_elem);
    }
    return clock_elem;
}

void clock_list_init(void)
{
    list_init(&clock_list);
    lock_init(&clock_list_lock);
    clock_elem=NULL;
}

void add_page(struct page* page)
{
    list_push_back(&clock_list, &(page->clock_elem));
}

void delete_page(struct page *page)
{
    if(&page->clock_elem==clock_elem) {
        clock_elem = list_next(clock_elem);
    }
    list_remove(&page->clock_elem);
}

/* When there's a shortage of physical pages, the clock algorithm is used to secure additional memory. */
void free_pages(enum palloc_flags alloc_flag)
{

    struct page *page;
    struct page *page_to_be_evicted;

    clock_elem=find_next_clock();

    page = list_entry(clock_elem, struct page, clock_elem);

    while(page->spte->pinned || pagedir_is_accessed(page->thread->pagedir, page->spte->vaddr))
    {
        pagedir_set_accessed(page->thread->pagedir, page->spte->vaddr, false);
        clock_elem=find_next_clock(); 
        page = list_entry(clock_elem, struct page, clock_elem);
    }
    page_to_be_evicted = page;

    switch(page_to_be_evicted->spte->type)
    {
        case ZERO:
            if(pagedir_is_dirty(page_to_be_evicted->thread->pagedir, page_to_be_evicted->spte->vaddr))
            {
                page_to_be_evicted->spte->swap_slot = swap_out(page_to_be_evicted->paddr);
                page_to_be_evicted->spte->type=SWAP;
            }
            break;
        case FILE:
            if(pagedir_is_dirty(page_to_be_evicted->thread->pagedir, page_to_be_evicted->spte->vaddr))
                file_write_at(page_to_be_evicted->spte->file, page_to_be_evicted->spte->vaddr, page_to_be_evicted->spte->read_bytes, page_to_be_evicted->spte->offset);
            break;
        case SWAP:
            page_to_be_evicted->spte->swap_slot = swap_out(page_to_be_evicted->paddr);
            break;
    }
    
    page_to_be_evicted->spte->is_loaded=false;

    free_page_helper (page_to_be_evicted);
}

struct page *allocate_page(enum palloc_flags alloc_flag)
{
    lock_acquire(&clock_list_lock);
    /* Allocate page using palloc_get_page(). */
    uint8_t *kpage = palloc_get_page(alloc_flag);
    while (kpage == NULL)
    {
        free_pages(alloc_flag);
        kpage=palloc_get_page(alloc_flag);
    }

    /* Initialize the struct page. */
    struct page *page=malloc(sizeof(struct page));
    page->paddr=kpage;
    page->thread=thread_current();

    /* Insert the page to the clock_list using add_page(). */
    add_page(page);
    lock_release(&clock_list_lock);

    return page;
}
void free_page(void *paddr)
{
    lock_acquire(&clock_list_lock); 
    struct page *page=NULL;

    /* Search for the struct page corresponding to the physicall address paddr in the clock_list. */
    struct list_elem *e = list_begin(&clock_list);
    while (e != list_end(&clock_list)) {
        struct page* free_page = list_entry(e, struct page, clock_elem);
        if (free_page->paddr == paddr) {
            page = free_page;
            break;
        }
        e = list_next(e);
    }
    

    /* If page is not null, call free_page_helper(). */
    if(page!=NULL)
        free_page_helper (page);

    lock_release(&clock_list_lock);
}
void free_page_helper (struct page *page)
{
    /* Delete from clock_list. */
    delete_page(page);
    /* Free the memory allocated to the struct page. */
    pagedir_clear_page (page->thread->pagedir, pg_round_down(page->spte->vaddr));
    palloc_free_page(page->paddr);
    free(page);
}

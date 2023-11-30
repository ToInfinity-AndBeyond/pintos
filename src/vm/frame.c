#include "frame.h"

static struct list_elem* find_next_clock()
{
    struct list_elem * next_elem = list_next(clock_elem);

    if(list_empty(&clock_list))
        return NULL;

    if(clock_elem == NULL || clock_elem == list_end(&clock_list) || next_elem == list_end(&clock_list))
    {
        return list_begin(&clock_list);
    }
    else
    {
        return next_elem;
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

struct page *allocate_page(enum palloc_flags alloc_flag)
{
    lock_acquire(&clock_list_lock);
    /* Allocate page using palloc_get_page(). */
    uint8_t *kpage = palloc_get_page(alloc_flag);
    while (kpage == NULL)
    {
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

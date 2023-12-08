#include "frame.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "lib/kernel/bitmap.h"

static struct list frame_table;
static struct lock frame_table_lock;
static struct list_elem *clock_elem;

static struct list_elem* find_next_clock(void)
{
    if(list_empty(&frame_table))
        return NULL;

    if(clock_elem == NULL || clock_elem == list_end(&frame_table))
    {
        return list_begin(&frame_table);
    }

    if (list_next(clock_elem) == list_end(&frame_table))
    {
        return list_begin(&frame_table);
    }
    else
    {
        return list_next(clock_elem);
    }
    return clock_elem;
}

void frame_table_init(void)
{
    list_init(&frame_table);
    lock_init(&frame_table_lock);
    clock_elem=NULL;
}

/* Add frame to the frame_table. */
void add_frame(struct frame *frame)
{
    list_push_back(&frame_table, &(frame->elem));
}

/* Delete frame from the frame_table. */
void delete_frame(struct frame *frame)
{
    if(&frame->elem == clock_elem) {
        clock_elem = find_next_clock();
    }
    list_remove(&frame->elem);
}

/* When there's a shortage of physical frames, the clock algorithm is used to secure additional memory. */
void evict_frames(void)
{
    struct frame *frame;
    struct frame *frame_to_be_evicted;

    clock_elem=find_next_clock();

    frame = list_entry(clock_elem, struct frame, elem);

    while(frame->spte->pinned || pagedir_is_accessed(frame->thread->pagedir, frame->spte->vaddr))
    {
        pagedir_set_accessed(frame->thread->pagedir, frame->spte->vaddr, false);
        clock_elem=find_next_clock(); 
        frame = list_entry(clock_elem, struct frame, elem);
    }
    frame_to_be_evicted = frame;

    /* Perform operations based on the type of spte. */
    switch(frame_to_be_evicted->spte->type)
    {
        case ZERO:
            if(pagedir_is_dirty(frame_to_be_evicted->thread->pagedir, frame_to_be_evicted->spte->vaddr))
            {
                size_t swap_slot = swap_out(frame_to_be_evicted->paddr);
                if (swap_slot == BITMAP_ERROR) {
                    PANIC("Ran out of swap slots");
                }

                frame_to_be_evicted->spte->swap_slot = swap_slot;
                frame_to_be_evicted->spte->type=SWAP;
            }
            break;
        case FILE:
            if(pagedir_is_dirty(frame_to_be_evicted->thread->pagedir, frame_to_be_evicted->spte->vaddr))
                file_write_at(frame_to_be_evicted->spte->file, frame_to_be_evicted->spte->vaddr, frame_to_be_evicted->spte->read_bytes, frame_to_be_evicted->spte->offset);
            break;
        case SWAP:
            frame_to_be_evicted->spte->swap_slot = swap_out(frame_to_be_evicted->paddr);
            break;
    }
    
    frame_to_be_evicted->spte->is_loaded=false;
    free_frame_helper (frame_to_be_evicted);
}

/* Allocate frame. */
struct frame *allocate_frame(enum palloc_flags alloc_flag)
{
    lock_acquire(&frame_table_lock);
    /* Allocate page using palloc_get_page(). */
    uint8_t *kpage = palloc_get_page(alloc_flag);
    while (kpage == NULL)
    {
        evict_frames();
        kpage=palloc_get_page(alloc_flag);
    }

    /* Initialize the struct frame. */
    struct frame *frame = malloc(sizeof(struct frame));
    frame->paddr = kpage;
    frame->thread = thread_current();

    /* Insert the frame to the frame_table using add_frame(). */
    add_frame(frame);
    lock_release(&frame_table_lock);

    return frame;
}
/* By traversing the frame_table, free the frame with corresponding paddr. */
void free_frame(void *paddr)
{
    lock_acquire(&frame_table_lock); 
    struct frame *frame = NULL;

    /* Search for the struct frame corresponding to the physical address paddr in the frame_table. */
    struct list_elem *elem = list_begin(&frame_table);
    while (elem != list_end(&frame_table)) {
        struct frame* free_frame = list_entry(elem, struct frame, elem);
        if (free_frame->paddr == paddr) {
            frame = free_frame;
            break;
        }
        elem = list_next(elem);
    }
    /* If frame is not null, call free_frame_helper(). */
    if(frame != NULL)
        free_frame_helper (frame);

    lock_release(&frame_table_lock);
}
/* Helper function to be used in freeing frames. */
void free_frame_helper (struct frame *frame)
{
    /* Delete from frame_table. */
    delete_frame(frame);
    /* Free the memory allocated to the struct frame. */
    pagedir_clear_page (frame->thread->pagedir, pg_round_down(frame->spte->vaddr));
    palloc_free_page(frame->paddr);
    free(frame);
}

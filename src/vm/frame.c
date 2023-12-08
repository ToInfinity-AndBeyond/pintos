#include "frame.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "lib/kernel/bitmap.h"
#include "userprog/process.h"

static struct list_elem* find_next_clock(void)
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

void frame_table_init(void)
{
    list_init(&clock_list);
    lock_init(&eviction_lock);
    lock_init(&clock_list_lock);
    clock_elem = NULL;
}

/* Add frame to the frame_table. */
void add_frame(struct frame *frame)
{
    list_push_back(&clock_list, &(frame->clock_elem));
}

/* Delete frame from the frame_table. */
void delete_frame(struct frame *frame)
{
    if(&frame->clock_elem==clock_elem) {
        clock_elem = list_next(clock_elem);
    }
    list_remove(&frame->clock_elem);
}

/* When there's a shortage of physical frames, the clock algorithm is used to secure additional memory. */
void evict_frames(void)
{
    lock_acquire(&eviction_lock);
    struct frame *frame;
    struct frame *frame_to_be_evicted;

    clock_elem=find_next_clock();

    frame = list_entry(clock_elem, struct frame, clock_elem);

    while(frame->spte->pinned || pagedir_is_accessed(frame->thread->pagedir, frame->spte->vaddr))
    {
        pagedir_set_accessed(frame->thread->pagedir, frame->spte->vaddr, false);
        clock_elem = find_next_clock(); 
        frame = list_entry(clock_elem, struct frame, clock_elem);
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

    lock_release(&eviction_lock);
}


/* Creates a frame with the given spte with the physical address of 
   a frame that loaded the same file. This spte doesn't have to be loaded, but has to be installed*/
struct frame *share_existing_page(struct spt_entry *spte) 
{
    if(spte->writable || spte->type != FILE){
        return NULL;
    }

    /* Iterate through the clock_list to find a frame loaded with the same file */
    struct list_elem *elem = list_begin(&clock_list);
    while (elem != list_end(&clock_list)) {
        struct frame* pg = list_entry(elem, struct frame, clock_elem);
        if (pg->spte->is_loaded && file_compare(pg->spte->file, spte->file)) {
            /* This frame loaded the same file, so it can be shared. 
               Setup the share_frame, which has the paddr of the loaded frame
               and spte of the argument */
            struct frame *share_frame = malloc(sizeof(struct frame));
            if(share_frame == NULL) // Failed malloc
                return NULL;
            share_frame->paddr = pg->paddr;
            share_frame->spte = spte;
            share_frame->thread = thread_current();
            /* Add this shared frame into the clock_list */
            add_frame(share_frame);

            lock_release(&clock_list_lock);
            return share_frame;
        }

        elem = list_next(elem);
    }

    /* Return false if not found */
    return NULL;
}

/* Allocate frame. */
struct frame *allocate_frame(enum palloc_flags alloc_flag)
{
    if (!lock_held_by_current_thread(&clock_list_lock))
        lock_acquire(&clock_list_lock);
        
    /* Allocate frame using palloc_get_page(). */
    uint8_t *kpage = palloc_get_page(alloc_flag);
    while (kpage == NULL)
    {
        evict_frames();
        kpage = palloc_get_page(alloc_flag);
    }

    /* Initialize the struct frame. */
    struct frame *frame = malloc(sizeof(struct frame));
    frame->paddr = kpage;
    frame->thread = thread_current();

    /* Insert the frame to the frame_table using add_frame(). */
    add_frame(frame);
    lock_release(&clock_list_lock);

    return frame;
}

/* By traversing the frame_table, free the frame with corresponding paddr. */
void free_frame(void *paddr)
{
    lock_acquire(&clock_list_lock); 
    if (!lock_held_by_current_thread(&eviction_lock))
        lock_acquire(&eviction_lock);

    struct frame *frame = NULL;

    /* Search for the struct page corresponding to the physicall address paddr in the clock_list. */
    struct list_elem *elem = list_begin(&clock_list);
    while (elem != list_end(&clock_list)) {
        struct frame* free_frame = list_entry(elem, struct frame, clock_elem);
        if (free_frame->paddr == paddr) {
            frame = free_frame;
            break;
        }
        elem = list_next(elem);
    }
    /* If frame is not null, call free_frame_helper(). */
    if(frame != NULL)
        free_frame_helper (frame);

    lock_release(&eviction_lock);
    lock_release(&clock_list_lock);
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

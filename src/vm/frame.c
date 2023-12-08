#include "frame.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "lib/kernel/bitmap.h"

static struct hash frame_table;
static struct lock frame_table_lock;

static struct hash_elem *clock_helem;
static struct hash_iterator clock_iterator;

static struct hash_elem* find_next_clock(void)
{
    if(hash_empty(&frame_table))
    {
        return NULL;
    }

    // Moves to the next and returns it
    struct hash_elem *new_clock = hash_next(&clock_iterator);

    if (clock_helem == NULL || new_clock == NULL)
    {
        // Go back to start
        hash_first(&clock_iterator, &frame_table);
        return hash_next(&clock_iterator);
    }

    return new_clock;
}

static unsigned frame_hash_func(const struct hash_elem *element, void *aux UNUSED)
{
    struct frame *frame = hash_entry(element, struct frame, helem);
    return hash_bytes(&frame->paddr, sizeof(frame->paddr));
}

static bool frame_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
    struct frame *frame_a = hash_entry(a, struct frame, helem);
    struct frame *frame_b = hash_entry(b, struct frame, helem);

    return frame_a->paddr < frame_b->paddr;
} 

void frame_table_init(void)
{
    hash_init(&frame_table, frame_hash_func, frame_less_func, NULL);
    hash_first(&clock_iterator, &frame_table);
    lock_init(&frame_table_lock);
    clock_helem=NULL;
}

/* Add frame to the frame_table. */
void add_frame(struct frame *frame)
{
    struct hash_elem *e = hash_insert(&frame_table, &frame->helem);
    ASSERT(e == NULL);
}

/* Delete frame from the frame_table. */
void delete_frame(struct frame *frame)
{
    if(&frame->helem == clock_helem) {
        clock_helem = find_next_clock();
    }
    // list_remove(&frame->helem);
    struct hash_elem *e = hash_delete(&frame_table, &frame->helem);
    ASSERT(e != NULL);
}

/* When there's a shortage of physical frames, the clock algorithm is used to secure additional memory. */
void evict_frames(void)
{
    struct frame *frame;
    struct frame *frame_to_be_evicted;

    clock_helem=find_next_clock();

    frame = hash_entry(clock_helem, struct frame, helem);

    while(frame->spte->pinned || pagedir_is_accessed(frame->thread->pagedir, frame->spte->vaddr))
    {
        pagedir_set_accessed(frame->thread->pagedir, frame->spte->vaddr, false);
        clock_helem = find_next_clock(); 
        frame = hash_entry(clock_helem, struct frame, helem);
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
        kpage = palloc_get_page(alloc_flag);
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
    struct hash_iterator i;
    hash_first(&i, &frame_table);
    while (hash_next(&i))
    {
        struct frame *free_frame = hash_entry(hash_cur(&i), struct frame, helem);
        if (free_frame->paddr == paddr)
        {
            frame = free_frame;
            break;
        }
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

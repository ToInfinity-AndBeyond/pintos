#include "vm/swap.h"
#include "devices/block.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "threads/vaddr.h"

struct lock swap_lock;
struct bitmap *swap_bitmap;
struct block *swap_block;

/* Initialize the swap slot. */
void swap_init(void)
{
    lock_init(&swap_lock);
    swap_block = block_get_role(BLOCK_SWAP);
    if (swap_block == NULL)
    {
        return;
    }
    swap_bitmap = bitmap_create(block_size(swap_block) * BLOCKS_PER_PAGE);
    if (swap_bitmap == NULL)
    {
        return;
    }
    bitmap_set_all(swap_bitmap, SWAP_ZERO);
}
/* Copy the data stored in the swap slot of used_index to physical address paddr. */
void swap_in(size_t used_index, void *paddr)
{
    lock_acquire(&swap_lock);
    if(bitmap_test(swap_bitmap, used_index))
    {
        for(int i = 0; i < BLOCKS_PER_PAGE; i++)
        {
            block_read(swap_block, BLOCKS_PER_PAGE*used_index+i, BLOCK_SECTOR_SIZE*i+paddr);
        }
        bitmap_reset(swap_bitmap, used_index);
        lock_release(&swap_lock);
    }
}
/* Record the page pointed to by the paddr in the swap partition. */
size_t swap_out(void *paddr)
{
    lock_acquire(&swap_lock);
    size_t swap_index = bitmap_scan(swap_bitmap, 0, 1, false);
    if(BITMAP_ERROR != swap_index)
    {
        for(int i = 0; i < BLOCKS_PER_PAGE; i++)
        {
            block_write(swap_block, BLOCKS_PER_PAGE*swap_index+i, BLOCK_SECTOR_SIZE*i+paddr);
        }
        bitmap_set(swap_bitmap, swap_index, true);
    }
    lock_release(&swap_lock);
    return swap_index;
}
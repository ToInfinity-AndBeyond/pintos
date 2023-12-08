#include "vm/page.h"
#include "vm/frame.h"
#include "lib/string.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

/* Using the vaddr of spt_entry as an argument, 
   return the hash value by using the hash_int() function. */
static unsigned spt_hash_func(const struct hash_elem *elem, void *aux UNUSED)
{
	struct spt_entry * spte = hash_entry(elem, struct spt_entry, elem);
	return hash_int((int) spte->vaddr);
}

/* Compare the vaddr values of the two input hash_elems. */ 
static bool spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct spt_entry *spte_a = hash_entry(a, struct spt_entry, elem);
	struct spt_entry *spte_b = hash_entry(b, struct spt_entry, elem);
	return spte_a->vaddr < spte_b->vaddr;
}

/* Initialize Hash Table using hash_init. */
void spt_init(struct hash *spt)
{
	hash_init(spt, spt_hash_func, spt_less_func, NULL);
}

void spte_initialize(struct spt_entry *spte, enum spt_page_type type, void *addr, 
					 struct file* file, bool writable, bool is_loaded, off_t offset, 
					 size_t page_read_bytes, size_t page_zero_bytes) {
    spte->type = type;
    spte->vaddr = addr;
    spte->writable = writable;
    spte->is_loaded = is_loaded;
    spte->file = file;
    spte->offset = offset;
    spte->read_bytes = page_read_bytes;
    spte->zero_bytes = page_zero_bytes;
}

/* Insert spt_entry using hash_insert() function. */
bool insert_spte(struct hash *spt, struct spt_entry *spte)
{
	spte->pinned = false;
	struct hash_elem* elem = hash_insert (spt, &(spte->elem));
	if(elem == NULL)
		return true;
	else
		return false;
}
/* Delete spt_entry from Hash Table using hash_delete() function. */
bool delete_spte(struct hash *spt, struct spt_entry *spte)
{
	struct hash_elem *elem = hash_delete(spt, &(spte->elem));

	if(elem == NULL)
		return false;
	else
	{
		/* free_page will acquire eviction_lock only if the thread is not holding it.
		   free_page will eventually release eviction_lock before its return */
		lock_acquire(&eviction_lock);
		free_page(pagedir_get_page (thread_current ()->pagedir, spte->vaddr));
		free(spte);
		return true;
	}
}

/* Search and return the spt_entry corresponding to the vaddr argument. */
struct spt_entry *find_spte(void *vaddr)
{
	struct thread *cur = thread_current();
	struct spt_entry spte;

	/* Get vaddr's page number using pg_round_down() function. */
	spte.vaddr = pg_round_down(vaddr);

	struct hash_elem* elem= hash_find(&(cur->spt), &(spte.elem));

	if(elem != NULL)
		return hash_entry(elem, struct spt_entry, elem);
	else
		return NULL;
}

/* A helper function to be used in spt_destory() function. */
static void spt_destroy_helper(struct hash_elem *elem, void *aux UNUSED)
{
	struct spt_entry *spte = hash_entry(elem, struct spt_entry, elem);
	lock_acquire(&eviction_lock);
	free_page(pagedir_get_page (thread_current ()->pagedir, spte->vaddr));
	free(spte);
}

/* Remove spt_entries from the hash table using the hash_destroy() function. */
void spt_destroy(struct hash *spt)
{
	hash_destroy(spt, spt_destroy_helper);
}

/* Loads pages existing on the disk into physical memory. */
bool load_file(void* paddr, struct spt_entry *spte)
{
	ASSERT(paddr != NULL);
	ASSERT(spte != NULL);
	ASSERT(spte->type == ZERO || spte->type == FILE);
	lock_acquire(&eviction_lock);
	int a = file_read_at(spte->file, paddr, spte->read_bytes, spte->offset);
	if (a != (int) spte->read_bytes)
	{
		return false;
	}
    memset (paddr + spte->read_bytes, 0, spte->zero_bytes);
	lock_release(&eviction_lock);
  	return true;
}
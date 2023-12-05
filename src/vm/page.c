#include "vm/page.h"
#include "vm/frame.h"
#include "lib/string.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

/* Using the vaddr of spt_entry as an argument, 
   return the hash value by using the hash_int() function. */
static unsigned spt_hash_func(const struct hash_elem *e, void *aux UNUSED)
{
	struct spt_entry * spte = hash_entry(e, struct spt_entry, elem);
	return hash_int((int) spte->vaddr);
}

/* Compare the vaddr values of the two input hash_elems. */ 
static bool spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	struct spt_entry *spte_a=hash_entry(a, struct spt_entry, elem);
	struct spt_entry *spte_b=hash_entry(b, struct spt_entry, elem);
	return spte_a->vaddr < spte_b->vaddr;
}

static void spt_destroy_func(struct hash_elem *e, void *aux UNUSED)
{
	struct spt_entry *spte=hash_entry(e, struct spt_entry, elem);
	free_page(pagedir_get_page (thread_current ()->pagedir, spte->vaddr));
	free(spte);
}

/* Initialize Hash Table using hash_init. */
void spt_init(struct hash *spt)
{
	hash_init (spt, spt_hash_func, spt_less_func, NULL);
}
/* Insert spt_entry using hash_insert() function. */
bool insert_spte(struct hash *spt, struct spt_entry *spte)
{
	spte -> pinned = false;
	struct hash_elem* elem = hash_insert (spt, &(spte->elem));
	if(elem ==NULL)
		return true;
	else
		return false;
}
/* Delete spt_entry from Hash Table using hash_delete() function. */
bool delete_spte(struct hash *spt, struct spt_entry *spte)
{
	struct hash_elem *elem =hash_delete(spt, &(spte->elem));

	if(elem ==NULL)
		return false;
	else
	{
		free_page(pagedir_get_page (thread_current ()->pagedir, spte->vaddr));
		free(spte);
		return true;
	}
}

/* Search and return the spt_entry corresponding to the vaddr argument. */
struct spt_entry *find_spte(void *vaddr)
{
	struct thread *cur=thread_current();
	struct spt_entry search_entry;

	/* Get vaddr's page number using pg_round_down() function. */
	search_entry.vaddr = pg_round_down(vaddr);

	struct hash_elem* e= hash_find(&(cur->spt), &(search_entry.elem));

	if(e!=NULL)
		return hash_entry(e, struct spt_entry, elem);
	else
		return NULL;
}
/* Remove spt_entries from the hash table using the hash_destroy() function. */
void spt_destroy(struct hash *spt)
{
	hash_destroy (spt, spt_destroy_func);
}

/* Loads pages existing on the disk into physical memory. */
bool load_file(void* paddr, struct spt_entry *spte){

    file_seek(spte->file, spte->offset);
    if(file_read (spte->file, paddr, spte->read_bytes) != (int)(spte->read_bytes))
	{
		return false;
	}
    memset (paddr + spte->read_bytes, 0, spte->zero_bytes);
  	return true;
}
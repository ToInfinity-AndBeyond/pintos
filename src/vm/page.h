#ifndef spt_PAGE_H
#define spt_PAGE_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"

enum spt_page_type {
  ZERO, /* Bits will be initialised to 0. */
  FILE, /* Bits will be read from file. */
  SWAP /* Bits will be swapped in from storage.  */
};

struct spt_entry{
  enum spt_page_type type; /* Holds location of page or if it is all zero */
  void *vaddr;  /* Virtual address of page */
  bool writable; /* Checks if page can be written to */
  
  bool is_loaded;  /* Checks if page is currently in memory */
  struct file* file; /* Used with mmap to identify file to copy from */

  struct list_elem mmap_elem; /* Allows insertion into mmap_entry */

  size_t offset;  /* Offset to write from file */
  size_t read_bytes; /* How many bytes to read and write from file */
  size_t zero_bytes; /* Number of bytes to set to 0 after reading from file */

  size_t swap_slot; /* Holds swap slot number of evicted pages */

  struct hash_elem elem; /* Allows insertion into supplemental page table */
};

struct mmap_entry {
  int mapid;  /* Identifies mmap_entry to remove upon call to sys_munmap() */
  struct file * file; /* File being mapped into memory */
  struct list_elem elem; /* Allows insertion into thread's mmap_list */
  struct list spte_list; /* Holds spt entries corresponding to this mmap */
};


void spt_init(struct hash *spt);
void spte_initialize(struct spt_entry *spte, enum spt_page_type type, void *addr,
                     struct file *file, bool writable, bool is_loaded, off_t offset,
                     size_t page_read_bytes, size_t page_zero_bytes);
bool insert_spte(struct hash *spt, struct spt_entry *spte);
bool delete_spte(struct hash *spt, struct spt_entry *spte);

struct spt_entry *find_spte(void *vaddr);
void spt_destroy(struct hash *spt);

bool load_file(void *paddr, struct spt_entry *spte);


#endif
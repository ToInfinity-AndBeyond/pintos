#ifndef spt_PAGE_H
#define spt_PAGE_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"


enum spt_page_type {
  ZERO,
  FILE,
  SWAP
};

struct spt_entry{
  enum spt_page_type type; 
  void *vaddr;  
  bool writable;
  bool pinned;
  
  bool is_loaded;  
  struct file* file; 

  struct list_elem mmap_elem; 

  size_t offset;  
  size_t read_bytes;
  size_t zero_bytes; 

  size_t swap_slot; 

  struct hash_elem elem;
};

struct mmap_entry {
  int mapid;  
  struct file * file; 
  struct list_elem elem; 
  struct list spte_list; 
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
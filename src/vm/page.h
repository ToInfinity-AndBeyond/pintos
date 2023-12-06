#ifndef spt_PAGE_H
#define spt_PAGE_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/vaddr.h"


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

// put hash_elem to find the hash interface
struct page {
  void *paddr;    
  struct spt_entry *spte;   
  struct thread *thread;  
  struct list_elem clock_elem;  
};

void spt_init(struct hash *spt);
bool insert_spte(struct hash *spt, struct spt_entry *spte);
bool delete_spte(struct hash *spt, struct spt_entry *spte);

struct spt_entry *find_spte(void *vaddr);
void spt_destroy(struct hash *spt);

bool load_file(void *paddr, struct spt_entry *spte);


#endif
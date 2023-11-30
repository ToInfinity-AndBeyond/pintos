#include <hash.h>
#include <list.h>
#include "threads/thread.h"


enum vm_page_type
  {
    SWAP,
    FILE,
    ZERO
  };

struct spt_entry{
    enum vm_page_type type; 
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

struct mmap_entry{
    int mapid;  
    struct file * file; 
    struct list_elem elem; 
    struct list spte_list; 
};

struct page{
    void *paddr;    
    struct spt_entry *spte;   
    struct thread *thread;  
    struct list_elem clock_elem;  
};




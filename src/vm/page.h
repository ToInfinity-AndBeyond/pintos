#include <hash.h>
#include <list.h>

struct spt_entry{
    uint8_t type; 
    void *vaddr;  
    bool writable;
      

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




#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/page.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void* stack_element (void *write_dest, void *write_src, int size);
bool check_stack_esp(void *addr, void *esp);
bool expand_stack(void *addr);
bool page_fault_helper(struct spt_entry *spte);

#endif /* userprog/process.h */

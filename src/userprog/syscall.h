#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define EXIT_ERROR -1

#include "userprog/process.h"
#include "threads/vaddr.h"

void syscall_init (void);
void exit(int status);

struct lock filesys_lock;

#endif /* userprog/syscall.h */

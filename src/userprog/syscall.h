#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"
#include "threads/vaddr.h"

void syscall_init (void);
void exit(int status);

#endif /* userprog/syscall.h */

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define MAX_ARGS 7

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) {
  uint32_t args[MAX_ARGS];
  
  check_ptr(f);
  int *syscall_args = {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1};
  int syscall_no = *(int *) f->esp;
  int syscall_ret;
  syscall_get_args(syscall_args[syscall_no], args, f);

  switch (syscall_no) {
    case SYS_HALT: 
      halt();
      break;
    case SYS_EXIT:
      exit((int) args[0]);
      break;
    case SYS_EXEC:
      syscall_ret = exec((const char *) args[0]);
      break;
    case SYS_WAIT:
      syscall_ret = wait((__pid_t) args[0]);
      break;
    case SYS_CREATE:
      syscall_ret = create((const char *) args[0], (unsigned) args[1]);
      break;
    case SYS_REMOVE:
      syscall_ret = remove((const char *) args[0]);
      break;
    case SYS_OPEN:
      syscall_ret = open((const char *) args[0]);
      break;
    case SYS_FILESIZE:
      syscall_ret = filesize((int) args[0]);
      break;
    case SYS_READ:
      syscall_ret = read((int) args[0], (void *) args[1], (unsigned) args[2]);
      break;
    case SYS_WRITE:
      syscall_ret = write((int) args[0], (void *) args[1], (unsigned) args[2]);
      break;
    case SYS_SEEK:
      seek((int) args[0], (unsigned) args[1]);
      break;
    case SYS_TELL:
      syscall_ret = tell((int) args[0]);
      break;
    case SYS_CLOSE:
      close((int) args[0]);
      break;
    }

  printf ("system call!\n");
  thread_exit ();
}

static void
syscall_get_args(int no_args, int *args, struct intr_frame *f) {
  for (int i = 0; i < no_args; i++) {
      args[i] = *(uint32_t *) (f->esp + (i + 1) * 4);
  }
}

void
check_ptr(void *ptr) {
  if (ptr == NULL){
    thread_exit();
  }
  if (!is_user_vaddr(ptr)) {
    thread_exit();
  }
  if (!pagedir_get_page(thread_current() -> pagedir, ptr)) {
    free(ptr);
    thread_exit();
  }
}
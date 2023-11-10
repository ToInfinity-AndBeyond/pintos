#include "userprog/syscall.h"
// #include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *f);
void syscall_init(void);

void check_ptr(void *ptr);

void halt(void);
void exit(uint32_t *esp);
int exec(uint32_t *esp);
int wait(uint32_t *esp);
int write(uint32_t *esp);
int read(uint32_t *esp);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f) {
  // int *syscall_args = {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1};
  int syscall_no = *(int *) f->esp;
  uint32_t *esp = (uint32_t *)f->esp;
  check_ptr(esp);

  switch (syscall_no) {
    case SYS_HALT: 
      halt();
      break;
    case SYS_EXIT:
      exit(esp);
      break;
    case SYS_EXEC:
      f->eax = exec(esp);
      break;
    case SYS_CREATE:
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_WAIT:
      f->eax = wait(esp);
      break;
    case SYS_READ:
      f->eax = read(esp);
      break;
    case SYS_WRITE:
      f->eax = write(esp);
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
  }
}

void
check_ptr(void *ptr) {
  // Free resources and terminate
  // if pointer is null / kernel vaddr / unmapped
  if (ptr == NULL || is_kernel_vaddr(ptr) || 
      !pagedir_get_page(thread_current() -> pagedir, ptr)) {
    thread_exit();
  }
}

void halt(void)
{
  shutdown_power_off();
}

void exit(uint32_t *esp)
{
  int status = (int)esp[1];
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit();  
}

int wait(uint32_t *esp)
{
  return process_wait((int)esp[1]);
}

int exec(uint32_t *esp)
{
  return process_execute((char *)esp[1]);
}

int write(uint32_t *esp)
{
  int fd = (int)esp[1];
  void *buffer = (void *)esp[2];
  unsigned size = (int)esp[3];
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1;
}

int read(uint32_t *esp)
{
  int fd = (int)esp[1];
  void *buffer = (void *)esp[2];
  unsigned size = (int)esp[3];
  if (fd == 0) {
    int i;
    for (i = 0; input_getc() || i <= size; ++i) {

    }
    return i;
  }
  return -1;
}



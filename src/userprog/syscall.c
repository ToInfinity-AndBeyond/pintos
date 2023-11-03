#include "userprog/syscall.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

// Could have pid_t and tid_t be the same
// Could have struct file * and fd be the same

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
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

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  printf("%s: exit(%d)", thread_current()->name, status);
  thread_exit();
}

// TODO: Need a semaphore here for ordering
pid_t exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

bool create(const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

bool remove(const char *file)
{
  return filesys_remove(file);
}

int write(int fd, const void *buffer, unsigned size)
{
  // return file_write(fd, buffer, size);
  return 0;
}
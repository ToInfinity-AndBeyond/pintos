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

// Many functions treat pid_t and tid_t the same (UTA confirmation this is fine)
// Many functions treat struct file * and fd the same

#define NO_CALLS 20
#define MAX_ARGS 10

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_get_args(int no_args, uint32_t *args, struct intr_frame *f) {
  for (int i = 0; i < no_args; i++) {
      args[i] = *(uint32_t *) (f->esp + (i + 1) * 4);
  }
}

static void
syscall_handler (struct intr_frame *f) {
  uint32_t args[MAX_ARGS];
  
  check_ptr(f);
  int syscall_args[NO_CALLS] = {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1};
  int syscall_no = *(int *) f->esp;
  uint32_t syscall_ret;
  syscall_get_args(syscall_args[syscall_no], args, f);

  switch (syscall_no) {
    case SYS_HALT: 
      halt();
      break;
    case SYS_EXIT:
      exit((int) args[0]);
      break;
    case SYS_EXEC:
      syscall_ret = (uint32_t) exec((const char *) args[0]);
      break;
    case SYS_WAIT:
      syscall_ret = (uint32_t) wait((pid_t) args[0]);
      break;
    case SYS_CREATE:
      syscall_ret = (uint32_t) create((const char *) args[0], (unsigned) args[1]);
      break;
    case SYS_REMOVE:
      syscall_ret = (uint32_t) remove((const char *) args[0]);
      break;
    case SYS_OPEN:
      syscall_ret = (uint32_t) open((const char *) args[0]);
      break;
    case SYS_FILESIZE:
      syscall_ret = (uint32_t) filesize((int) args[0]);
      break;
    case SYS_READ:
      syscall_ret = (uint32_t) read((int) args[0], (void *) args[1], (unsigned) args[2]);
      break;
    case SYS_WRITE:
      syscall_ret = (uint32_t) write((int) args[0], (void *) args[1], (unsigned) args[2]);
      break;
    case SYS_SEEK:
      seek((int) args[0], (unsigned) args[1]);
      break;
    case SYS_TELL:
      syscall_ret = (uint32_t) tell((int) args[0]);
      break;
    case SYS_CLOSE:
      close((int) args[0]);
      break;
    }

  f->eax = syscall_ret;

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
  struct thread *t = thread_current();
  printf("%s: exit(%d)\n", t->name, status);
  thread_exit();  
}

// TODO: Need a semaphore here for ordering
pid_t exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

// Uses int process_wait(tid_t child_tid)
int wait(pid_t pid)
{
  struct thread *t = thread_current();
  struct thread *relevant_child;
  bool found = false;

  struct list_elem *e;
  for (e = list_begin(&t->children_list); e != list_end(&t->children_list); e = list_next(e))
  {
    struct thread *child = list_entry(e, struct thread, child_elem);
    if (child->tid == pid)
    {
      relevant_child = child;
      found = true;
    }
  }

  if (!found || relevant_child->parent_is_waiting)
  {
    return -1;
  }

  relevant_child->parent_is_waiting = true;

  // Seems as if I could do this at the beginning so this is incorrect
  return process_wait(relevant_child->tid);
}

bool create(const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}

bool remove(const char *file)
{
  return filesys_remove(file);
}

int open(const char *file)
{
  return filesys_open(file);
}

int filesize(int fd)
{
  return file_length(fd);
}

int read(int fd, void *buffer, unsigned size)
{
  return file_read(fd, buffer, size);
}

int write(int fd, const void *buffer, unsigned size)
{
  return file_write(fd, buffer, size);
}

void seek(int fd, unsigned position)
{
  file_seek(fd, position);
}

unsigned tell(int fd)
{
  return file_tell(fd);
}

void close(int fd)
{
  file_close(fd);
}

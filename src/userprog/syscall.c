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
#include "devices/input.h"

static void syscall_handler (struct intr_frame *f);
void syscall_init(void);

void check_ptr(void *esp);

void halt(void);
void exit(int status);
int exec(const char *cmd_line);
int wait(int pid);
int write(int fd, const void *buffer, unsigned size);
int read(int fd, void *buffer, unsigned size);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) {
  int syscall_args[]= {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1};
  int syscall_no = *(int *) f->esp;
  uint32_t *esp = (uint32_t *)f->esp;
  check_ptr(esp);

  switch (syscall_no) {
    case SYS_HALT: 
      halt();
      break;
    case SYS_EXIT:
      exit((int)esp[1]);
      break;
    case SYS_EXEC:
      f->eax = exec((const char *)esp[1]);
      break;
    case SYS_CREATE:
      f -> eax = create((const char *)esp[1], (unsigned)esp[2]);
      break;
    case SYS_REMOVE:
      f -> eax = remove((const char *)esp[1]);
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      f->eax = filesize((int)esp[1]);
      break;
    case SYS_WAIT:
      f->eax = wait((pid_t)esp[1]);
      break;
    case SYS_READ:
      f->eax = read((int)esp[1], (void *)esp[2], (unsigned)esp[3]);
      break;
    case SYS_WRITE:
      f->eax = write((int)esp[1], (const void *)esp[2], (unsigned)esp[3]);
      break;
    case SYS_SEEK:
      seek((int)esp[1], (unsigned)esp[2]);
      break;
    case SYS_TELL:
      f->eax = tell((int)esp[1]);
      break;
    case SYS_CLOSE:
      break;
  }
}

void
check_ptr(void *ptr) {
  if (ptr == NULL || is_kernel_vaddr(ptr) || 
      !pagedir_get_page(thread_current() -> pagedir, ptr)) {
    thread_exit();
  }
}

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit();  
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

pid_t exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

bool create(const char *file, unsigned initial_size)
{
  if (file == NULL)
  {
    exit(-1);
  }
  return filesys_create(file, initial_size);
}

bool remove (const char *file)
{
  if (file == NULL) 
  {
    exit(-1);
  }
  return filesys_remove(file);
}

int filesize (int fd)
{
  return file_length(thread_current() -> fd[fd]);
}

int read(int fd, void *buffer, unsigned size)
{
  if (fd == 0) {
    int i;
    for (i = 0; input_getc() || i <= size; ++i) {

    }
    return i;
  }
  return -1;
}

int write(int fd, const void *buffer, unsigned size)
{
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1;
}

void seek(int fd, unsigned position)
{
  file_seek(thread_current() -> fd[fd], position);
}

unsigned tell(int fd)
{
  return file_tell(thread_current() -> fd[fd]);
} 




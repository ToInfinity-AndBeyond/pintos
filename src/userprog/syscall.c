#include "userprog/syscall.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define FD_BEGIN 3
#define FD_END 128


static struct lock filesys_lock;
static const int syscall_args[]= {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1};

static void syscall_handler (struct intr_frame *f);
void syscall_init(void);

void
syscall_init (void) 
{
  /* When syscall is initialised, lock_init. */
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Checks if virtual address is correct*/
static void check_pointer(uint32_t *esp, int args_num)
{
  if (!pagedir_get_page(thread_current() -> pagedir, esp))
  {
    exit(-1);
  }
  for (int i = 0; i <= args_num; ++i)
  {
    if (!is_user_vaddr((void *)esp[i]))
    {
      exit(-1);
    }
  }
}

/* Using switch - case, invokes a system call functions. */
static void
syscall_handler (struct intr_frame *f) {
  /* Array used to store the number of syscall's arguments*/
  int syscall_no = *(int *) f->esp;
  uint32_t *esp = (uint32_t *)f->esp; // stack pointer esp
  check_pointer(esp, syscall_args[syscall_no]);

  /* Each system call function is invoked by referencing esp as a parameter. */
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
      f->eax = create((const char *)esp[1], (unsigned)esp[2]);
      break;
    case SYS_REMOVE:
      f->eax = remove((const char *)esp[1]);
      break;
    case SYS_OPEN:
      f->eax = open((const char *)esp[1]);
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
      close ((int)esp[1]);
      break;
    default:
      exit(-1);
  }
}

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  /* Set exit status. */
  thread_current()->parent_relation->exit_status = status;  

  /* In order to prevent memory leak, closed all the files using file_close */
  for (int i = FD_BEGIN; i < FD_END; i++)
  {
    if (thread_current() -> fd[i] != NULL)
    {
      file_close(thread_current() -> fd[i]);
    }
  }
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
  /* lock_acquire and lock_released are used before and after 
     file operation is performed, respectively*/
  lock_acquire(&filesys_lock);
  bool is_created = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return is_created;
}

bool remove (const char *file)
{
  /* If file name is null, terminate. */
  if (file == NULL) 
  {
    exit(-1);
  }
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  bool is_removed = filesys_remove(file);
  lock_release(&filesys_lock);
  return is_removed;
}

int open (const char *file)
{
  /* If file name is null, terminate. */
  if (file == NULL)
  {
    exit(-1);
  }
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  struct file *fp = filesys_open(file);
  lock_release(&filesys_lock);
  if (fp == NULL)
  {
    return -1;
  }

  for (int i = FD_BEGIN; i < FD_END; i++)
  {
    if (thread_current() -> fd[i] == NULL)
    {
      /* Prevent writing to the current thread */
      if (!strcmp(thread_name(), file))
      {
        file_deny_write(fp);
      }
      thread_current() -> fd[i] = fp;
      return i;
    }
  }
  return -1;
}

int filesize (int fd)
{
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  int file_size = file_length(thread_current() -> fd[fd]);
  lock_release(&filesys_lock);
  return file_size;
}

int read(int fd, void *buffer, unsigned size)
{
  if (fd == STDIN) {
    unsigned i;
    for (i = 0; input_getc() || i <= size; ++i) {

    }
    return i;
  }
  else if (fd >= FD_BEGIN && fd < FD_END)
  {
    lock_acquire(&filesys_lock);
    int read_val = file_read(thread_current() -> fd[fd], buffer, size);
    lock_release(&filesys_lock);
    return read_val;
  }
  else
  {
    exit(-1);
  }
}

int write(int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT) 
  {
    lock_acquire(&filesys_lock);
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  } 
  else if (fd >= FD_BEGIN && fd < FD_END)
  {
    lock_acquire(&filesys_lock);
    int write_val = file_write(thread_current() -> fd[fd], buffer, size);
    lock_release(&filesys_lock);
    return write_val;
  }
  else
  {
    exit(-1);
  }
  return -1;
}

void seek(int fd, unsigned position)
{
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  file_seek(thread_current() -> fd[fd], position);
  lock_release(&filesys_lock);
}

unsigned tell(int fd)
{
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  unsigned tell_val = file_tell(thread_current() -> fd[fd]);
  lock_release(&filesys_lock);
  return tell_val;
} 

void close(int fd)
{
  if (fd < 2 || fd >= 128) {
    exit(-1);
  }
  struct file *fp = thread_current() -> fd[fd];
  thread_current() -> fd[fd] = NULL;
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  file_close(fp);
  lock_release(&filesys_lock);
}

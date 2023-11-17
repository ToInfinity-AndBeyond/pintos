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

#define VOID_RET 0

uint32_t sys_halt (uint32_t *esp);
uint32_t sys_exit (uint32_t *esp);
uint32_t sys_exec (uint32_t *esp);
uint32_t sys_wait (uint32_t *esp);
uint32_t sys_create (uint32_t *esp);
uint32_t sys_remove (uint32_t *esp);
uint32_t sys_open (uint32_t *esp);
uint32_t sys_filesize (uint32_t *esp);
uint32_t sys_read (uint32_t *esp);
uint32_t sys_write (uint32_t *esp);
uint32_t sys_seek (uint32_t *esp);
uint32_t sys_tell (uint32_t *esp);
uint32_t sys_close (uint32_t *esp);

void exit (int status);

static struct lock filesys_lock;
static const int syscall_args[] = {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1};
static uint32_t (*syscall_func[]) (uint32_t *esp) = 
{
  sys_halt,
  sys_exit,
  sys_exec,
  sys_wait,
  sys_create,
  sys_remove,
  sys_open,
  sys_filesize,
  sys_read,
  sys_write,
  sys_seek,
  sys_tell,
  sys_close
};
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
    exit(EXIT_ERROR);
  }
  for (int i = 0; i <= args_num; ++i)
  {
    if (!is_user_vaddr((void *)esp[i]))
    {
      exit(EXIT_ERROR);
    }
  }
}

/* Using function pointers, invokes a system call functions. */
static void
syscall_handler (struct intr_frame *f) {
  /* Array used to store the number of syscall's arguments. */
  int syscall_no = *(int *) f->esp;
  uint32_t *esp = (uint32_t *)f->esp; /* Stack pointer esp. */
  check_pointer(esp, syscall_args[syscall_no]);
  f->eax = (*syscall_func[syscall_no])(esp); 
}

/* Exit used for process.c and exception.c. */
void exit (int status) 
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

uint32_t sys_halt (uint32_t *esp UNUSED)
{
  shutdown_power_off();
  return VOID_RET;
}

/* Exit function called by syscall handler (function pointer). */
uint32_t sys_exit (uint32_t *esp)
{
  int status = (int) esp[1];
  exit(status);
}

uint32_t sys_wait (uint32_t *esp)
{
  pid_t pid = (pid_t) esp[1];

  return process_wait(pid);
}

uint32_t sys_exec (uint32_t *esp)
{
  const char *cmd_line = (const char *) esp[1];

  return process_execute(cmd_line);
}

uint32_t sys_create (uint32_t *esp)
{
  const char *file = (const char *) esp[1];
  unsigned initial_size = (unsigned) esp[2];
  
  if (file == NULL)
  {
    exit(EXIT_ERROR);
  }
  /* lock_acquire and lock_released are used before and after 
     file operation is performed, respectively*/
  lock_acquire(&filesys_lock);
  bool is_created = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return is_created;
}

uint32_t sys_remove (uint32_t *esp)
{
  const char *file = (const char *) esp[1];

  /* If file name is null, terminate. */
  if (file == NULL) 
  {
    exit(EXIT_ERROR);
  }
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  bool is_removed = filesys_remove(file);
  lock_release(&filesys_lock);
  return is_removed;
}

uint32_t sys_open (uint32_t *esp)
{
  const char *file = (const char *) esp[1];

  /* If file name is null, terminate. */
  if (file == NULL)
  {
    exit(EXIT_ERROR);
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

uint32_t sys_filesize (uint32_t *esp)
{
  int fd = (int) esp[1];

  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  int file_size = file_length(thread_current() -> fd[fd]);
  lock_release(&filesys_lock);
  return file_size;
}

uint32_t sys_read (uint32_t *esp)
{
  int fd = (int) esp[1];
  void *buffer = (void *) esp[2];
  unsigned size = (unsigned) esp[3];

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
    exit(EXIT_ERROR);
  }
}

uint32_t sys_write (uint32_t *esp)
{
  int fd = (int) esp[1];
  const void *buffer = (const void *) esp[2];
  unsigned size = (unsigned) esp[3];

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
    exit(EXIT_ERROR);
  }
  return -1;
}

uint32_t sys_seek (uint32_t *esp)
{
  int fd = (int) esp[1];
  unsigned position = (unsigned) esp[2];

  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  file_seek(thread_current() -> fd[fd], position);
  lock_release(&filesys_lock);
  return VOID_RET;
}

uint32_t sys_tell (uint32_t *esp)
{
  int fd = (int) esp[1];

  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  unsigned tell_val = file_tell(thread_current() -> fd[fd]);
  lock_release(&filesys_lock);
  return tell_val;
} 

uint32_t sys_close (uint32_t *esp)
{
  int fd = (int) esp[1];

  if (fd < FD_BEGIN || fd >= FD_END) 
  {
    exit(EXIT_ERROR);
  }
  struct file *fp = thread_current() -> fd[fd];
  thread_current() -> fd[fd] = NULL;
  /* Synchronization using lock */
  lock_acquire(&filesys_lock);
  file_close(fp);
  lock_release(&filesys_lock);
  return VOID_RET;
}

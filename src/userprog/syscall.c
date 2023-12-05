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
#include "vm/frame.h"
#include "vm/page.h"
#include "devices/swap.h"

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
uint32_t sys_mmap (uint32_t *esp);
uint32_t sys_munmap (uint32_t *esp);


void exit (int status);

static const int syscall_args[] = {0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1, 2, 1};
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
  sys_close,
  sys_mmap,
  sys_munmap
};
static void syscall_handler (struct intr_frame *f);
void syscall_init(void);

void
check_spte_address(void *str, unsigned size, void *esp)
{
    for (int i = 0; i < size; i++)
    {
        if (!is_user_vaddr(str + i))
        {
            exit(EXIT_ERROR);
        }

        struct spt_entry *spte = find_spte(str + i);
        if (spte == NULL)
        {
            if (!check_stack_esp(str + i, esp))
                exit(EXIT_ERROR);
            expand_stack(str + i);
            spte = find_spte(str + i);
        }

        if (spte == NULL)
            exit(EXIT_ERROR);
    }
}

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
  check_spte_address(buffer, size, esp);

  /* If fd == STDIN, reads from the keyboard using input_getc() */
  if (fd == STDIN) {
    unsigned i;
    for (i = 0; input_getc() || i <= size; ++i) {

    }
    return i;
  } /* If fd is between FD_BEGIN and FD_END, reads size bytes from the file open as fd into buffer. */
  else if (fd >= FD_BEGIN && fd < FD_END)
  {
    lock_acquire(&filesys_lock);
    int read_val = file_read(thread_current() -> fd[fd], buffer, size);
    lock_release(&filesys_lock);
    return read_val;
  } /* Otherwise, exit. */
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
  check_spte_address(buffer, size, esp);

  /* If fd == STDOUT, writes to the console using putbuf(). */
  if (fd == STDOUT) 
  {
    lock_acquire(&filesys_lock);
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  } /* If fd is between FD_BEGIN and FD_END, writes size bytes from buffer to the open file fd. */
  else if (fd >= FD_BEGIN && fd < FD_END)
  {
    lock_acquire(&filesys_lock);
    int write_val = file_write(thread_current() -> fd[fd], buffer, size);
    lock_release(&filesys_lock);
    return write_val;
  } /* Otherwise, exit. */
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

/* Load file data into memory by demand paging, 
   return the mapid upon success, return -1 upon failure. */
uint32_t sys_mmap(uint32_t *esp)
{
  int fd = (int) esp[1];
  void *addr = (void*) esp[2];
  int mapid;
  struct file *fp = thread_current() -> fd[fd];
  size_t offset = 0;

  /* Invalid cases. */
  if (fp == NULL || pg_ofs(addr) != 0 || !addr || !is_user_vaddr || find_spte(addr))
  {
    return -1;
  }

  /* Initialize mmap_entry, and if failed, return -1
     This is done separately from the invalid cases, as mmape must be freed */
  struct mmap_entry *mmape = malloc(sizeof(struct mmap_entry));
  if (mmape == NULL) 
  {
    return -1;
  }

  mapid = thread_current() -> next_mapid++;
  mmape->mapid = mapid;
  lock_acquire(&filesys_lock);
  mmape->file = file_reopen(fp);
  lock_release(&filesys_lock);
  list_init (&mmape->spte_list);
  list_push_back(&thread_current() -> mmap_list, &mmape->elem);

  /* spt_entry Initialization. */
  int read_bytes_size = file_length(mmape -> file);
  while(read_bytes_size > 0)
  {
    size_t page_read_bytes = read_bytes_size < PGSIZE ? read_bytes_size : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;
    
    struct spt_entry *spte = malloc(sizeof(struct spt_entry));
    spte->type = FILE;
    spte->vaddr = addr;
    spte->writable = true;
    spte->is_loaded = false;
    spte->file = mmape->file;
    spte->offset = offset;
    spte->read_bytes = page_read_bytes;
    spte->zero_bytes = page_zero_bytes;
    list_push_back(&(mmape->spte_list), &(spte->mmap_elem));
    insert_spte(&thread_current() -> spt, spte);

    addr += PGSIZE;
    offset += page_read_bytes;
    read_bytes_size -= page_read_bytes;
  }
  return mapid;
}

/* Release all spt entries in mmap_list that have a corresponding mapid */
uint32_t sys_munmap(uint32_t *esp)
{
  int mapid = (int)esp[1];

  struct thread *cur= thread_current();

  struct mmap_entry *mmape=NULL;
  for (struct list_elem *e = list_begin(&cur->mmap_list); e != list_end(&cur->mmap_list); e = list_next(e)) {
    struct mmap_entry *mmap_entry = list_entry(e, struct mmap_entry, elem);
    if (mmap_entry->mapid == mapid) {
      mmape = mmap_entry;
      break;
    }
  }
  if (mmape == NULL)
    return VOID_RET;

  /* Delete spt_entry, page table entry, mmap_file. */
  for(struct list_elem *e = list_begin(&mmape->spte_list); e != list_end(&mmape->spte_list);)
  {

    struct list_elem *next_e = list_next(e);

    struct spt_entry *spte = list_entry(e, struct spt_entry, mmap_elem);
    if(spte->is_loaded && pagedir_is_dirty(thread_current()->pagedir, spte->vaddr)) {
      file_write_at(spte->file, spte->vaddr, spte->read_bytes, spte->offset);
      }
      spte->is_loaded = false;
      list_remove(e);

      delete_spte(&thread_current()->spt, spte);
      e=next_e;
    }
    list_remove(&mmape->elem);
    free(mmape);

}

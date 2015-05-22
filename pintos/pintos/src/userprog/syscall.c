#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
// $bunch of includes
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

#define USER_VADDR_BOTTOM ((void *) 0x08048000)

int process_add_file(struct file *f);
struct file* process_get_file(int fd);

static void syscall_handler (struct intr_frame *);
int user_to_kernel_ptr(const void *vaddr);
void get_arg (struct intr_frame *f, int *arg, int n);

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void halt (void){
  shutdown_power_off();
}

void exit (int status){ 
  struct thread *t = thread_current();
  if (thread_live(t->parent) && t->child){
      t->child->status = status;
  }
  printf("%s: exit(%d)\n", t->name, status);
  thread_exit();
}

pid_t exec (const char *cmd_line)
{
  pid_t pid = process_execute(cmd_line);
  struct child_process* cp = get_child_process(pid);
  if (!cp)
    {
      return -1;
    }
  while(!cp->load){
    barrier();
  }
  if (cp->load == 2)
    {
      return -1;
    }
  return pid;
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return success;
}

bool remove (const char *file)
{
  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

int open (const char *file)
{
  lock_acquire(&file_lock);
  struct file *f = filesys_open(file);
  if (!f)
    {
      lock_release(&file_lock);
      return -1;
    }
  int fd = process_add_file(f);
  lock_release(&file_lock);
  return fd;
}

int filesize (int fd)
{
  lock_acquire(&file_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&file_lock);
      return -1;
    }
  int size = file_length(f);
  lock_release(&file_lock);
  return size;
}

int read (int fd, void *buffer, unsigned size)
{
  if (fd == STDIN_FILENO)
    {
      unsigned i;
      uint8_t* local_buffer = (uint8_t *) buffer;
      for (i = 0; i < size; i++)
  {
    local_buffer[i] = input_getc();
  }
      return size;
    }
  lock_acquire(&file_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&file_lock);
      return -1;
    }
  int bytes = file_read(f, buffer, size);
  lock_release(&file_lock);
  return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO)
    {
      putbuf(buffer, size);
      return size;
    }
  lock_acquire(&file_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&file_lock);
      return -1;
    }
  int bytes = file_write(f, buffer, size);
  lock_release(&file_lock);
  return bytes;
}

void seek (int fd, unsigned position)
{
  lock_acquire(&file_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&file_lock);
      return;
    }
  file_seek(f, position);
  lock_release(&file_lock);
}

unsigned tell (int fd)
{
  lock_acquire(&file_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&file_lock);
      return -1;
    }
  off_t offset = file_tell(f);
  lock_release(&file_lock);
  return offset;
}

void close (int fd)
{
  lock_acquire(&file_lock);
  process_close_file(fd);
  lock_release(&file_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int i, arg[MAX_ARGS];
  for (i = 0; i < MAX_ARGS; i++)
    {
      arg[i] = * ((int *) f->esp + i);
    }
  switch (arg[0])
    {
    case SYS_HALT:
      {
  halt(); 
  break;
      }
    case SYS_EXIT:
      {
  exit(arg[1]);
  break;
      }
    case SYS_EXEC:
      {
  arg[1] = user_to_kernel_ptr((const void *) arg[1]);
  f->eax = exec((const char *) arg[1]); 
  break;
      }
    case SYS_WAIT:
      {
  f->eax = wait(arg[1]);
  break;
      }
    case SYS_CREATE:
      {
  arg[1] = user_to_kernel_ptr((const void *) arg[1]);
  f->eax = create((const char *)arg[1], (unsigned) arg[2]);
  break;
      }
    case SYS_REMOVE:
      {
  arg[1] = user_to_kernel_ptr((const void *) arg[1]);
  f->eax = remove((const char *) arg[1]);
  break;
      }
    case SYS_OPEN:
      {
  arg[1] = user_to_kernel_ptr((const void *) arg[1]);
  f->eax = open((const char *) arg[1]);
  break;    
      }
    case SYS_FILESIZE:
      {
  f->eax = filesize(arg[1]);
  break;
      }
    case SYS_READ:
      {
  arg[2] = user_to_kernel_ptr((const void *) arg[2]);
  f->eax = read(arg[1], (void *) arg[2], (unsigned) arg[3]);
  break;
      }
    case SYS_WRITE:
      { 
  arg[2] = user_to_kernel_ptr((const void *) arg[2]);
  f->eax = write(arg[1], (const void *) arg[2],
           (unsigned) arg[3]);
  break;
      }
    case SYS_SEEK:
      {
  seek(arg[1], (unsigned) arg[2]);
  break;
      } 
    case SYS_TELL:
      { 
  f->eax = tell(arg[1]);
  break;
      }
    case SYS_CLOSE:
      { 
  close(arg[1]);
  break;
      } 
    }
}

int user_to_kernel_ptr(const void *vaddr)
{
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr || !is_user_vaddr(vaddr)){
      thread_exit();
     return 0;
  }
  return (int) ptr;
}

struct child_process* add_child_process (int pid)
{
  struct child_process* child = malloc(sizeof(struct child_process));
  if (!child){
    return NULL;
  }
  child->pid = pid;
  child->load = 0; // $FIXED load failure = 0
  child->wait = false;
  child->exit = false;
  list_push_back(&thread_current()->child_list, &child->elem);
  return child;
}

struct child_process* get_child_process (int pid)
{
  struct thread *t = thread_current();
  struct list_elem *e;

  for (e = list_begin (&t->child_list); e != list_end (&t->child_list); e = list_next (e)){
    struct child_process *child = list_entry(e, struct child_process, elem);
    if (pid == child->pid){
      return child;
    }
  }
  return NULL;
}

void remove_child_process (struct child_process *cp)
{
  list_remove(&cp->elem);
  free(cp);
}

void remove_child_processes (void)
{
  struct thread *t = thread_current();
  struct list_elem *next, *e = list_begin(&t->child_list);

  while (e != list_end (&t->child_list))
    {
      next = list_next(e);
      struct child_process *cp = list_entry (e, struct child_process,
               elem);
      list_remove(&cp->elem);
      free(cp);
      e = next;
    }
}

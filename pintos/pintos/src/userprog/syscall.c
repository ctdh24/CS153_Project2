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

static void syscall_handler (struct intr_frame *);
int user_to_kernel_ptr(const void *vaddr);
void get_arg (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
void check_valid_buffer (void* buffer, unsigned size);
void check_valid_string (const void* str);

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
  if (cp->load == 0)
    {
      sema_down(&cp->load_sema);
    }
  if (cp->load == 2)
    {
      remove_child_process(cp);
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
  bool success = file_create(file, initial_size);
  lock_release(&file_lock);
  return success;
}

bool remove (const char *file)
{
  lock_acquire(&file_lock);
  bool success = file_remove(file);
  lock_release(&file_lock);
  return success;
}

int open (const char *file)
{
  lock_acquire(&file_lock);
  struct file *f = file_open(file);
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
syscall_handler (struct intr_frame *f)
{/*
	unsigned callNum;
	int args[3];
	int numOfArgs;
	
	//##Get syscall number
	copy_in (&call_nr, f->esp, sizeof callNum);

	//##Using the number find out which system call is being used
	numOfArgs = number of args that system call uses {0,1,2,3}
	
	
	copy_in (args, (uint32_t *) f->esp + 1, sizeof *args * numOfArgs);
	
	//##Use switch statement or something and run this below for each
	//##Depending on the callNum...
	f->eax = desired_sys_call_fun (args[0], args[1], args[2]);
*/
  int arg[3];
  int esp = user_to_kernel_ptr((const void*) f->esp);
  switch (* (int *) esp)
    {
    case SYS_HALT:
      {
  halt(); 
  break;
      }
    case SYS_EXIT:
      {
  get_arg(f, &arg[0], 1);
  exit(arg[0]);
  break;
      }
    case SYS_EXEC:
      {
  get_arg(f, &arg[0], 1);
  check_valid_string((const void *) arg[0]);
  arg[0] = user_to_kernel_ptr((const void *) arg[0]);
  f->eax = exec((const char *) arg[0]); 
  break;
      }
    case SYS_WAIT:
      {
  get_arg(f, &arg[0], 1);
  f->eax = wait(arg[0]);
  break;
      }
    case SYS_CREATE:
      {
  get_arg(f, &arg[0], 2);
  check_valid_string((const void *) arg[0]);
  arg[0] = user_to_kernel_ptr((const void *) arg[0]);
  f->eax = create((const char *)arg[0], (unsigned) arg[1]);
  break;
      }
    case SYS_REMOVE:
      {
  get_arg(f, &arg[0], 1);
  check_valid_string((const void *) arg[0]);
  arg[0] = user_to_kernel_ptr((const void *) arg[0]);
  f->eax = remove((const char *) arg[0]);
  break;
      }
    case SYS_OPEN:
      {
  get_arg(f, &arg[0], 1);
  check_valid_string((const void *) arg[0]);
  arg[0] = user_to_kernel_ptr((const void *) arg[0]);
  f->eax = open((const char *) arg[0]);
  break;    
      }
    case SYS_FILESIZE:
      {
  get_arg(f, &arg[0], 1);
  f->eax = filesize(arg[0]);
  break;
      }
    case SYS_READ:
      {
  get_arg(f, &arg[0], 3);
  check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
  arg[1] = user_to_kernel_ptr((const void *) arg[1]);
  f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
  break;
      }
    case SYS_WRITE:
      { 
  get_arg(f, &arg[0], 3);
  check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
  arg[1] = user_to_kernel_ptr((const void *) arg[1]);
  f->eax = write(arg[0], (const void *) arg[1],
           (unsigned) arg[2]);
  break;
      }
    case SYS_SEEK:
      {
  get_arg(f, &arg[0], 2);
  seek(arg[0], (unsigned) arg[1]);
  break;
      } 
    case SYS_TELL:
      { 
  get_arg(f, &arg[0], 1);
  f->eax = tell(arg[0]);
  break;
      }
    case SYS_CLOSE:
      { 
  get_arg(f, &arg[0], 1);
  close(arg[0]);
  break;
      }
    }
}

void check_valid_ptr (const void *vaddr)
{
  if (!is_user_vaddr(vaddr) || vaddr < USER_VADDR_BOTTOM)
    {
      exit(-1);
    }
}

int user_to_kernel_ptr(const void *vaddr)
{
  check_valid_ptr(vaddr);
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
    {
      exit(-1);
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
  sema_init(&child->load_sema, 0);
  sema_init(&child->exit_sema, 0);
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

void get_arg (struct intr_frame *f, int *arg, int n)
{
  int i;
  int *ptr;
  for (i = 0; i < n; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_valid_ptr((const void *) ptr);
      arg[i] = *ptr;
    }
}

void check_valid_buffer (void* buffer, unsigned size)
{
  unsigned i;
  char* local_buffer = (char *) buffer;
  for (i = 0; i < size; i++)
    {
      check_valid_ptr((const void*) local_buffer);
      local_buffer++;
    }
}

void check_valid_string (const void* str)
{
  while (* (char *) user_to_kernel_ptr(str) != 0)
    {
      str = (char *) str + 1;
    }
}

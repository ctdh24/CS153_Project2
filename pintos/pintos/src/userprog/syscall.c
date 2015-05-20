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

static void syscall_handler (struct intr_frame *);

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

//pid_t exec (const char *cmd_line) {}
int wait();
//int wait (pid_t pid) {}
bool create (const char *file, unsigned initial_size) {}
bool remove (const char *file) {}
int open (const char *file) {}
int filesize (int fd) {}
int read (int fd, void *buffer, unsigned size) {}
int write (int fd, const void *buffer, unsigned size){
	/*
	Pseudo code for Write system call()
	case SYS_WRITE:
	{ 
		 get_arg(f, &arg[0], 3);
		 check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
		 arg[1] = user_to_kernel_ptr((const void *) arg[1]);
		 f->eax = write(arg[0], (const void *) arg[1],
				(unsigned) arg[2]);
		 break;
	}
	*/
}
void seek (int fd, unsigned position) {}
unsigned tell (int fd) {}
void close (int fd) {}

/* Copies a byte from user address USRC to kernel address DST.
   USRC must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
   // $MOVED above get_user/copy_in/copy_in_string to get rid of compile conflict
static inline bool
get_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}


/* Copies SIZE bytes from user address USRC to kernel address
   DST.
   Call thread_exit() if any of the user accesses are invalid. */
static void
copy_in (void *dst_, const void *usrc_, size_t size) 
{
  uint8_t *dst = dst_;
  const uint8_t *usrc = usrc_;
 
  for (; size > 0; size--, dst++, usrc++) 
    if (usrc >= (uint8_t *) PHYS_BASE || !get_user (dst, usrc)) 
      thread_exit ();
}


static bool
setup_stack (void **esp)//##Add cmd_line here 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      //## uint8_t *upage = ( (uint8_t *) PHYS_BASE ) - PGSIZE;
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true); //##Change the first parameter to upage since its the same thing.
      if (success)
        *esp = PHYS_BASE; //## Remove this, setup_stack_helper will set esp
	//## success = setup_stack_helper(...)
      else
        palloc_free_page (kpage);
    }
  return success;
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

/* Creates a copy of user string US in kernel memory
   and returns it as a page that must be freed with
   palloc_free_page().
   Truncates the string at PGSIZE bytes in size.
   Call thread_exit() if any of the user accesses are invalid. */
static char *
copy_in_string (const char *us) 
{
  char *ks;
  size_t length;
 
  ks = palloc_get_page (0);
  if (ks == NULL) 
    thread_exit ();
 
  for (length = 0; length < PGSIZE; length++)
    {
      if (us >= (char *) PHYS_BASE || !get_user (ks + length, us++)) 
        {
          palloc_free_page (ks);
          thread_exit (); 
        }
       
      if (ks[length] == '\0')
        return ks;
    }
  ks[PGSIZE - 1] = '\0';
  return ks;
}


/* Returns true if UADDR is a valid, mapped user address,
   false otherwise. */
static bool
verify_user (const void *uaddr) 
{
  return (uaddr < PHYS_BASE
          && pagedir_get_page (thread_current ()->pagedir, uaddr) != NULL);
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

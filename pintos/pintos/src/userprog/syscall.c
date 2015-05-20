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

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
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
{
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


/* Copies a byte from user address USRC to kernel address DST.
   USRC must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static inline bool
get_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}




/* Returns true if UADDR is a valid, mapped user address,
   false otherwise. */
static bool
verify_user (const void *uaddr) 
{
  return (uaddr < PHYS_BASE
          && pagedir_get_page (thread_current ()->pagedir, uaddr) != NULL);
}
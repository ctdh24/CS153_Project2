#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2

struct child_process {
  int pid;
  int load;
  bool wait;
  bool exit;
  int status;
  struct lock wait_lock;
  struct list_elem elem;
};

struct lock file_lock;

void syscall_init (void);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_line) ;
int wait (pid_t pid) ;
bool create (const char *file, unsigned initial_size) ;
bool remove (const char *file) ;
int open (const char *file) ;
int filesize (int fd) ;
int read (int fd, void *buffer, unsigned size) ;
int write (int fd, const void *buffer, unsigned size) ;
void seek (int fd, unsigned position) ;
unsigned tell (int fd) ;
void close (int fd) ;

static void syscall_handler (struct intr_frame *);

int user_to_kernel_ptr (const void *vaddr);
static void copy_in (void *dst_, const void *usrc_, size_t size);
static char *copy_in_string (const char *us);
static inline bool get_user (uint8_t *dst, const uint8_t *usrc);
static bool verify_user (const void *uaddr);

struct child_process* add_child (int pid);
struct child_process* get_child (int pid);
void remove_child_process (struct child_process *cp);
void remove_child_processes (void);
#endif /* userprog/syscall.h */

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock file_lock;

struct child_process {
  int pid;
  int status;
  int load;

  bool wait;
  bool exit;
  
  struct semaphore load_sema;
  struct semaphore exit_sema;
  struct list_elem elem;
};

void syscall_init (void);

void halt (void);
void exit (int status);
//pid_t exec (const char *cmd_line) ;
int wait();
//int wait (pid_t pid) ;
bool create (const char *file, unsigned initial_size) ;
bool remove (const char *file) ;
int open (const char *file) ;
int filesize (int fd) ;
int read (int fd, void *buffer, unsigned size) ;
int write (int fd, const void *buffer, unsigned size) ;
void seek (int fd, unsigned position) ;
unsigned tell (int fd) ;
void close (int fd) ;

#endif /* userprog/syscall.h */

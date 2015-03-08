#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <uthash.h>

static void syscall_handler (struct intr_frame *);

static struct lock syscall_lock;

static struct lock status_lock;

static struct list status_list;

void halt_h(void);
void exit_h (int status);
tid_t exec_h (const char *cmd_line);

void
syscall_init (void) 
{

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&status_lock);
  list_init(&status_list);

}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");

  int *esp_int_pointer = (int*) f->esp;
	
  int syscall_number = *esp_int_pointer;
  switch (syscall_number) 
  {
  	case SYS_HALT:
  		halt_h ();
  	  break;
  	case SYS_EXIT:
			{
				int status;
				status = (int)*(esp_int_pointer+1);
				exit_h (status);
			}
  		break;
  	case SYS_EXEC:
			{
				const char *cmd_line = (char *) (esp_int_pointer+1);
				exec_h (cmd_line);
			}
  		break;
		/*
  	case SYS_WAIT:
  		wait_h ();
  		break;
		case SYS_CREATE:
			create_h ();
			break;
		case SYS_REMOVE:
			remove_h ();
			break;
		case SYS_OPEN:
			open_h ();
			break;
		case SYS_FILESIZE:
			filesize_h ();
			break;
		case SYS_READ:
			read_h ();
			break;
		case SYS_WRITE:
			write_h ();
			break;
		case SYS_SEEK:
			seek_h ();
			break;
		case SYS_TELL:
			tell_h ();
			break;
		case SYS_CLOSE:
			close_h ();
			break;
*/
  }
  thread_exit ();
}

void
halt_h (void)
{
	lock_acquire(&syscall_lock);
	shutdown_power_off();
	lock_release(&syscall_lock);
}

void
exit_h (int status)
{
		struct status_holder new_status;
		new_status.tid = thread_current()->tid;
		new_status.status = status;
		new_status.ptr_to_parent = thread_current()->parent;
		lock_acquire(&status_lock);
		list_push_front(&status_list, &new_status.status_elem);
		lock_release(&status_lock);
		thread_exit ();
}

tid_t
exec_h (const char *cmd_line)
{
	lock_acquire(&syscall_lock);
	tid_t tid = process_execute(cmd_line);
	lock_release(&syscall_lock);
	return tid;
}

int
wait_h (tid_t tid)
{
	return process_wait(tid);
}
/*
bool
create_h (const char *file, unsigned initial_size) 
{

}

bool 
remove_h (const char *file)
{

}

int 
open_h (const char *file)
{

}

int
filesize_h (int d) 
{

}

int 
read_h (int fd, void *buffer, unsigned size)
{

}

int 
write_h (int fd, const void *buffer, unsigned size)
{

}

void 
seek_h (int fd, unsigned position)
{

}

unsigned
tell_h (int fd)
{

}

void 
close_h (int fd)
{

}
*/

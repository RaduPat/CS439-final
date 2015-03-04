#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

static struct lock syscall_lock;

void
syscall_init (void) 
{

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");

  int *esp_int_pointer = (int*) f->esp;
  int syscall_number = *esp_int_pointer;
  switch(syscall_number) 
  {
  	case SYS_HALT:
  		halt_h();
  	  break;
  	case SYS_EXIT:
  		exit_h();
  		break;
  	case SYS_EXEC:
  		exec_h();
  		break;
  	case SYS_WAIT;
  		wait_h();
  		break;
  }

  thread_exit ();
}

void
halt_h(void)
{
	lock_acquire(&syscall_lock);
	shutdown_power_off();
	lock_release(&syscall_lock);
}

void
exit_h(int status)
{
	
}

tid_t
exec_h(const char *cmd_line)
{
	lock_acquire(&syscall_lock);
	tid_t tid = process_execute(cmd_line);
	lock_release(&syscall_lock);
	return tid;
}


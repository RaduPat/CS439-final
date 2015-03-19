#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);

void halt_h(void);
void exit_h (int status);
tid_t exec_h (char *cmd_line);
int wait_h (tid_t tid);
bool create_h (char *file, unsigned initial_size);
bool remove_h (char *file);
int open_h (char *file);
int filesize_h (int file_descriptor);
int read_h (int file_descriptor, void *buffer, unsigned size);
int write_h (int file_descriptor, void *buffer, unsigned size);
int seek_h (int file_descriptor, unsigned position);
unsigned tell_h (int file_descriptor);
int close_h (int file_descriptor);

struct file * find_open_file (int fd);

static struct lock syscall_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&syscall_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int *esp_int_pointer = (int*) f->esp;
	
  int syscall_number = *esp_int_pointer;
  switch (syscall_number) 
  {
  	
  	case SYS_HALT:
  		{
 			halt_h ();
  	  	}
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
			f->eax = exec_h (cmd_line);
		}
  		break;
  	case SYS_WAIT:
  		{
  			tid_t tid = (tid_t) *(esp_int_pointer+1);
  			f->eax = wait_h(tid);
  		}
  		break;
  	case SYS_CREATE:
  		{
  			const char *file = (char *) *(esp_int_pointer+1);
  			unsigned initial_size = (unsigned) *(esp_int_pointer+2);
  			f->eax = create_h(file, initial_size);
  		}
  	case SYS_REMOVE:
  		{
  			const char *file = (char *) *(esp_int_pointer+1);
  			f->eax = remove_h(file);
  		}
  		break;
  	case SYS_OPEN:
  		{
  			const char *file = (char *) *(esp_int_pointer+1);
  			f->eax = open_h(file);
  		}
  		break;
  	case SYS_FILESIZE:
  		{
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			f->eax = filesize_h(file_descriptor);
  		}
  		break;
  	case SYS_READ:
  		{
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			void *buffer = (void *) *(esp_int_pointer+2);
  			unsigned size = (unsigned) *(esp_int_pointer+3);
  			f->eax = read_h (file_descriptor, buffer, size);
  		}
  		break;
  	case SYS_WRITE:
  		{
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			const void *buffer = (void *) *(esp_int_pointer+2);
  			unsigned size = (unsigned) *(esp_int_pointer+3);
  			f->eax = write_h (file_descriptor, buffer, size);
  		}
  		break;
  	case SYS_SEEK:
  		{
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			unsigned pos = (unsigned) *(esp_int_pointer+2);
  			f->eax = seek_h (file_descriptor, pos);
  		}
  	case SYS_TELL:
  		{
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			f->eax = tell_h (file_descriptor);
  		}
  	case SYS_CLOSE:
  		{
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			f->eax = close_h (file_descriptor);
  		}
  }
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
	thread_current()->stat_holder->status = status;
	thread_exit();
}

tid_t
exec_h (char *cmd_line)
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

bool
create_h (char *file, unsigned initial_size) 
{
	lock_acquire(&syscall_lock);
	bool success = filesys_create(file, initial_size);
	lock_release(&syscall_lock);
	return success;
}

bool
remove_h (char *file)
{
	lock_acquire(&syscall_lock);
	bool success = filesys_remove(file);
	lock_release(&syscall_lock);
	return success;
}

int
open_h (char *file)
{
	lock_acquire(&syscall_lock);
	struct file *open_file = filesys_open(file);
	lock_release(&syscall_lock);
	open_file->fd = (thread_current()->allocate_fd)++;
	list_push_back (&thread_current()->open_files, &open_file->open_elem);
	return open_file->fd;
}

int 
filesize_h (int file_descriptor)
{
	struct file *found_file = find_open_file (file_descriptor);
	int file_size;
	if(found_file != NULL)
		file_size = file_length (found_file);
	else
		file_size = -1;
	return file_size;
}

int
read_h (int file_descriptor, void *buffer, unsigned size)
{
	if(file_descriptor == STDIN_FILENO)
		{
			uint8_t *new_buffer = (uint8_t *) buffer;
			int i;
			for (i = 0; i < size; i++)
				{
					*new_buffer = input_getc ();
					new_buffer++;
				}
			return (int) size;
		}
	else
		{
			int bytes_read = -1;
			struct file *found_file = find_open_file (file_descriptor);
			if(found_file != NULL) 
			{
				lock_acquire (&syscall_lock);
				bytes_read = file_read (found_file, buffer, size);
				lock_release (&syscall_lock);
			}
			return bytes_read;
		}
}

int
write_h (int file_descriptor, void *buffer, unsigned size)
{
	if(file_descriptor == STDOUT_FILENO)
		{
			putbuf ((char *) buffer, (size_t) size);
			return (int) size;
		}
	else
		{
			int bytes_written = -1;
			struct file *found_file = find_open_file (file_descriptor);
			if (found_file != NULL)
				{
					lock_acquire (&syscall_lock);
					bytes_written = file_write (found_file, buffer, size);
					lock_release (&syscall_lock);
				}
			return bytes_written;
		}
}

int
seek_h (int file_descriptor, unsigned position) 
{
	struct file *found_file = find_open_file (file_descriptor);
	if(found_file != NULL)
	{
		found_file->pos = position;
		return 1; //returning 1 and -1 to signify pushing to EAX
	}
	return -1;
}

unsigned
tell_h (int file_descriptor)
{
	struct file *found_file = find_open_file (file_descriptor);
	if(found_file != NULL)
	{
		return found_file->pos;
	}
	return -1;
}

int
close_h (int file_descriptor)
{
	struct file *found_file = find_open_file (file_descriptor);
	if(found_file != NULL)
		list_remove(&found_file->open_elem);
	else
		return -1;

	free(found_file);
	return 1;
}

struct file *
find_open_file (int fd) 
{
	struct list_elem * e;
  	for (e = list_begin (&thread_current()->open_files); e != list_end (&thread_current()->open_files); e = list_next (e))
    	{
    		struct file *temp_file = list_entry (e, struct file, open_elem);
    		if(temp_file->fd == fd)
    			return temp_file;
    	}
    return NULL;
}



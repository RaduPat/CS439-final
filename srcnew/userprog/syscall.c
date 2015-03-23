#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

#define MAX_FILES 128

static void syscall_handler (struct intr_frame *);

void set_denywrite (bool);


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
void check_pointer (void *pointer);

struct file * find_open_file (int fd);

/* lock to synchronize access to the filesystem */
static struct lock syscall_lock;

/* list of all open files and a lock to synchronize access to it */
//static struct list all_open_files;
static struct lock lock_allopenfiles;

/* array to keep track of open files globally */
struct file * all_open_files[MAX_FILES];  
int index_glob; 

void
syscall_init (void) 
{
  index_glob = 0;
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&syscall_lock);
  lock_init(&lock_allopenfiles);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  check_pointer(f->esp);

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
			check_pointer(esp_int_pointer+1);
			status = (int) *(esp_int_pointer+1);
			exit_h (status);
		}
		break;

  	case SYS_EXEC:
		{
			check_pointer(esp_int_pointer+1);
			const char **cmd_line = (char **) (esp_int_pointer+1);
			f->eax = exec_h (*cmd_line);
		}
  		break;
  	case SYS_WAIT:
  		{	
  			check_pointer(esp_int_pointer+1);
  			tid_t tid = (tid_t) *(esp_int_pointer+1);
  			f->eax = wait_h(tid);
  		}
  		break;
  	case SYS_CREATE:
  		{
  			check_pointer(esp_int_pointer+1);
  			check_pointer(esp_int_pointer+2);
  			const char **file = (char **) (esp_int_pointer+1);
  			unsigned initial_size = (unsigned) *(esp_int_pointer+2);
  			f->eax = create_h(*file, initial_size);
  			if (f->eax)	
  			{
  				//printf("^^^^^^^^^^^ file create success in switch\n");
  			}
  			else{
  				//printf("^^^^^^^^^^^ file create failed in switch\n");
  			}
  		}
  	case SYS_REMOVE:
  		{
  			check_pointer(esp_int_pointer+1);
  			const char **file = (char **) (esp_int_pointer+1);
  			f->eax = remove_h(*file);
  		}
  		break;
  	case SYS_OPEN:
  		{
  			check_pointer(esp_int_pointer+1);
  			const char **file = (char **)(esp_int_pointer+1);
  			f->eax = open_h(*file);
  		}
  		break;
  	case SYS_FILESIZE:
  		{
  			check_pointer(esp_int_pointer+1);
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			f->eax = filesize_h(file_descriptor);
  		}
  		break;
  	case SYS_READ:
  		{
  			check_pointer(esp_int_pointer+1);
  			check_pointer(esp_int_pointer+2);
  			check_pointer(esp_int_pointer+3);
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			void **buffer = (void **) (esp_int_pointer+2);
  			unsigned size = (unsigned) *(esp_int_pointer+3);
  			f->eax = read_h (file_descriptor, *buffer, size);
  		}
  		break;
  	case SYS_WRITE:
  		{
  			check_pointer(esp_int_pointer+1);
  			check_pointer(esp_int_pointer+2);
  			check_pointer(esp_int_pointer+3);
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			const void **buffer = (void **) (esp_int_pointer+2);
  			unsigned size = (unsigned) *(esp_int_pointer+3);
  			f->eax = write_h (file_descriptor, *buffer, size);
  		}
  		break;
  	case SYS_SEEK:
  		{
  			check_pointer(esp_int_pointer+1);
  			check_pointer(esp_int_pointer+2);
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			unsigned pos = (unsigned) *(esp_int_pointer+2);
  			f->eax = seek_h (file_descriptor, pos);
  		}
  	case SYS_TELL:
  		{
  			check_pointer(esp_int_pointer+1);
  			int file_descriptor = (int) *(esp_int_pointer+1);
  			f->eax = tell_h (file_descriptor);
  		}
  	case SYS_CLOSE:
  		{
  			check_pointer(esp_int_pointer+1);
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
	check_pointer(cmd_line);
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
	check_pointer(file);
	printf("^^^^^^^^ file name %s***********\n", file);
	bool success = false;
	lock_acquire(&syscall_lock);
	printf("lock acquire\n");
	success = filesys_create(file, (off_t) initial_size);
	printf("lock release\n");
	lock_release(&syscall_lock);

	printf("success = %d\n", success);
	return success;
}

bool
remove_h (char *file)
{
	check_pointer(file);
	lock_acquire(&syscall_lock);
	bool success = filesys_remove(file);
	lock_release(&syscall_lock);
	return success;
}

int
open_h (char *file)
{
	check_pointer(file);
	lock_acquire(&syscall_lock);
	struct file *open_file = filesys_open(file);
	lock_release(&syscall_lock);
	if (open_file == NULL){
		return -1;
	}
	if (isAliveThread(file))
	{
		file_deny_write(open_file);
	}
	//strlcpy (open_file->name, file, sizeof open_file->name);
	
	assign_fd(thread_current () -> index_fd, thread_current () -> open_files);
	lock_acquire(&lock_allopenfiles);
	assign_fd(index_glob, all_open_files);
	lock_release(&lock_allopenfiles);
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
	check_pointer(buffer);
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
	check_pointer(buffer);
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
					printf("Number of bytes written in Handler: %d", bytes_written);
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
		thread_current () -> open_files[file_descriptor] = NULL;
	else
		return -1;
	all_open_files[file_descriptor] = NULL;
	//free(found_file);
	return 1;
}

struct file *
find_open_file (int fd) 
{
	return thread_current ()->open_files[fd]; 
}

void
check_pointer (void *pointer)
{
	//void *success = pagedir_get_page (thread_current()->pagedir, pointer);
	//check above phys base			check within its own page
	if(is_kernel_vaddr(pointer) || pagedir_get_page (thread_current()->pagedir, pointer) == NULL)
		exit_h(-1);
	//exit_h will handle freeing the page and closing the process
}

/*Function to iterate through an array of files to assign a file descriptor.
Should loop around to the beginning of the array upon reaching the last index*/
void
assign_fd(int fd, file * files)
{
	int i;
	for(i = 2; i < sizeof (files); i++)
	{
		if(files[i] == NULL)
			files[i] = fd;
		if(i == sizeof - 1)
			i = 2;
	}
}

void
set_denywrite(bool shouldSet_denywrite) {
	lock_acquire(&lock_allopenfiles);
	struct list_elem * e;
	int i;

  	for (i = 0; i< sizeof(all_open_files); i++)
    	{
    		struct file *f = all_open_files[i];

    		if(!strcmp(thread_current()->name, f->name)){
    			if (shouldSet_denywrite)
    			{
    				file_deny_write(f);
    			}
    			else{
    				file_allow_write(f);
    			}
    		}
    	}
	lock_release(&lock_allopenfiles);
}

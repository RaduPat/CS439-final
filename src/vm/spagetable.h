#ifndef VM_SPAGETABLE_H
#define VM_SPAGETABLE_H

#include <stdbool.h>
#include <debug.h>
#include <inttypes.h>
#include <stddef.h>
#include "filesys/off_t.h"
#include "lib/kernel/list.h"

/* Eddy drove here */
/* Used to determine how to load page back into memory */
enum load_instruction
{
	USELESS = -1,
	FILE = 0,
	SWAP = 1,
	STACK = 2
};

/* Struct that holds supplemental information for a page. This will
	serve as an entry in the supplemental page table. */
struct spinfo
{
	struct file *file; 									/* Pointer to the file if that is where we need to load from */
	off_t file_offset;
	size_t bytes_to_read;         			/* The number of bytes we need to read from the file */
	bool writable; 											/* Whether or not the file can be written to */
	uint8_t * upage_address;						/* Address of the user page */
	uint8_t * kpage_address;						/* Address of the kernel page containing this page */
	struct list_elem sptable_elem; 			/* List element for the supplemental page table. */
	enum load_instruction instructions; /*Enum for the load instructions */
	int index_into_swap;				/* an index into the block in swapspace which contains this page */
};

struct spinfo * find_spinfo (struct list * info_list, uint8_t * page);

#endif /* vm/spagetable.h */

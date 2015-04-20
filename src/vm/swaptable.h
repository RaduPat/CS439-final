#ifndef VM_SWAPTABLE_H
#define VM_SWAPTABLE_H

#include <stdbool.h>
#include <debug.h>
#include "vm/spagetable.h"

/* Nicholas drove here */
//meta data for an entry in the swap space 
struct metaswap_entry
{
	int swap_index;					/* Denotes the offset in the swap block */	
	bool isfilled;					/* Is this entry in the swap table occupied by a page or not? */
	enum load_instruction instructions_for_pageheld; // tells us the kind of page held
	bool isdirty;					/* tells us if the page that's swapped in is dirty */
};

//initialize the swaptable
void swaptable_init(void);
//get the metaswap_entry at an index within the swap table
struct metaswap_entry* get_metaswap_entry_byindex(int index);
//get the next available metaswap_entry within the swap table, returns the index which can be used to access it.
struct metaswap_entry* next_empty_metaswap_entry(void);
//move the data in *page into the swap space
int move_into_swap(void* page, enum load_instruction instruction, bool isdirty);
//read data from the swapspace at swaptable[index] into the *page 
void read_from_swap(int index, void *page);
//free up a swap entry
struct metaswap_entry* free_metaswap_entry(int index);

#endif /* vm/swaptable.h */

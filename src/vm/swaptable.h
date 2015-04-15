#ifndef VM_SWAPTABLE_H
#define VM_SWAPTABLE_H

#include <stdbool.h>
#include <debug.h>
#include "vm/spagetable.h"

//meta data for an entry in the swap space 
struct metaswap_entry
{
	int swap_index;					/* Denotes the offset in the swap block */	
	bool isfilled;					/* Is this entry in the swap table occupied by a page or not? */
	enum load_instruction instructions_for_pageheld; // tells us the kind of page held
};

//get the metaswap_entry at an index within the swap table
struct metaswap_entry* get_metaswap_entry_byindex((int index);
//get the next available metaswap_entry within the swap table, returns the index which can be used to access it.
int metaswap_entry* next_empty_metaswap_entry(void);
//assign a page for the swap entry
void* assign_swap_page(void);
//free up a swap entry
void free_swap_entry(void* page);

#endif /* vm/swaptable.h */

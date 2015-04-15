#include "swaptable.h"
#include "block.h"
#include "threads/palloc.h"
#include <inttypes.h>


//extern uint32_t init_ram_pages;
//const uint32_t user_frames = 1024; //init_ram_pages/2;

struct metaswap_entry *swaptable;
struct block *swap_block;
size_t swaptable_size;

void
swaptable_init()
{
	swap_block = block_get_role(BLOCK_SWAP);
	swaptable_size = swap_block->size/(PG_SIZE/BLOCK_SECTOR_SIZE);
	swaptable = calloc(swaptable_size, sizeof(struct metaswap_entry));
	int i;
	for(i = 0; i < swaptable_size; i++)
	{
		swaptable[i].swap_index = i;
		swaptable[i].isfilled = false;
	}
}

struct metaswap_entry* 
get_metaswap_entry_byindex(int index)
{
	return &swaptable[index];
}

void
set_type_of_page_swapped_in(int index, enum load_instruction type){
	swaptable[index].instructions_for_pageheld = type;
}

struct metaswap_entry* 
next_empty_metaswap_entry()
{
	int i;
	for(i=0; i<swaptable_size; i++){
		if(!swaptable[i].isfilled)
			return &swaptable[i];
	}
	PANIC("No more swap space!!!!");
	return NULL;
}

int 
move_into_swap(void* page){
	struct metaswap_entry* new_swap_entry = next_empty_metaswap_entry();
	if(new_swap_entry==NULL)
		PANIC("no empty swap entries available");

	char * buffer = (char*) page;
	int i;
	for(i = 0; i < 8; i++)
	{
		buffer += BLOCK_SECTOR_SIZE * i;
		block_write(swap_block, 8 * new_swap_entry->swap_index + i, buffer);
	}
	new_swap_entry->isfilled = true;
	return new_swap_entry->swap_index;
}

void
read_from_swap(int index, void *page)
{
	char * buffer = (char*) page;
	int i;
	for(i = 0; i < 8; i++)
	{
		buffer += BLOCK_SECTOR_SIZE * i;
		block_read(swap_block, 8 * index + i, buffer);
	}
}

void
free_metaswap_entry(int index){
	struct metaswap_entry* swap_entry2free = get_metaswap_entry_byindex(index);
	ASSERT(swap_entry2free->isfilled);
	swap_entry2free->isfilled = false;
}

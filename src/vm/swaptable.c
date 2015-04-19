#include "swaptable.h"
#include "devices/block.h"
#include "threads/palloc.h"
#include <inttypes.h>
#include "threads/vaddr.h"
#include "threads/synch.h"


//extern uint32_t init_ram_pages;
//const uint32_t user_frames = 1024; //init_ram_pages/2;

struct metaswap_entry *swaptable;
struct block *swap_block;
uint32_t swaptable_size;

void
swaptable_init()
{
	swap_block = block_get_role(BLOCK_SWAP);
	if(swap_block == NULL)
		PANIC("Swap_block == NULL");
	swaptable_size = swap_block->size/(PGSIZE/BLOCK_SECTOR_SIZE);
	swaptable = calloc(swaptable_size, sizeof(struct metaswap_entry));
	int i;
	for(i = 0; i < swaptable_size; i++)
	{
		swaptable[i].swap_index = i;
		swaptable[i].isfilled = false;
		swaptable[i].instructions_for_pageheld = USELESS;
		swaptable[i].isdirty = false;
	}
}

struct metaswap_entry* 
get_metaswap_entry_byindex(int index)
{
	ASSERT(swaptable_size > index);
	ASSERT(index >= 0);
	return &swaptable[index];
}

struct metaswap_entry* 
next_empty_metaswap_entry()
{
	int i;
	for(i=0; i<swaptable_size; i++){
		if(!swaptable[i].isfilled) {
			return &swaptable[i];
		}
	}
	PANIC("No more swap space!!!!");
	return NULL;
}

int 
move_into_swap(void* page, enum load_instruction instruction, bool isdirty) {
	if(page == NULL)
		PANIC("Page in move_into_swap == NULL");
	struct metaswap_entry* new_swap_entry = next_empty_metaswap_entry();
	if(new_swap_entry==NULL)
		PANIC("no empty swap entries available");

	char * buffer = (char*) page;
	int i;
	for(i = 0; i < 8; i++)
	{
		block_write(swap_block, 8 * new_swap_entry->swap_index + i, buffer);
		buffer += BLOCK_SECTOR_SIZE;
	}
	new_swap_entry->isfilled = true;
	new_swap_entry->instructions_for_pageheld = instruction;
	new_swap_entry->isdirty = isdirty;
	return new_swap_entry->swap_index;
}

void
read_from_swap(int index, void *page)
{
	ASSERT(index != -1);
	if(page == NULL)
		PANIC("Page in read_from_swap == NULL");
	char * buffer = (char*) page;
	int i;
	for(i = 0; i < 8; i++)
	{
		block_read(swap_block, 8 * index + i, buffer);
		buffer += BLOCK_SECTOR_SIZE;
	}
}

struct metaswap_entry*
free_metaswap_entry(int index){
	ASSERT(index != -1);
	struct metaswap_entry* swap_entry2free = get_metaswap_entry_byindex(index);
	if(swap_entry2free == NULL)
		PANIC("swap_entry2free == NULL");
	ASSERT(swap_entry2free->isfilled);
	swap_entry2free->isfilled = false;
	swap_entry2free->instructions_for_pageheld = USELESS;
	return swap_entry2free;
}

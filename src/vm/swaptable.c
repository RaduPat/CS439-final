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
struct lock swap_lock;

void
swaptable_init()
{
	lock_init(&swap_lock);
	swap_block = block_get_role(BLOCK_SWAP);
	swaptable_size = swap_block->size/(PGSIZE/BLOCK_SECTOR_SIZE);
	swaptable = calloc(swaptable_size, sizeof(struct metaswap_entry));
	int i;
	for(i = 0; i < swaptable_size; i++)
	{
		swaptable[i].swap_index = i;
		swaptable[i].isfilled = false;
		swaptable[i].instructions_for_pageheld = USELESS;
	}
}

struct metaswap_entry* 
get_metaswap_entry_byindex(int index)
{
	return &swaptable[index];
}

struct metaswap_entry* 
next_empty_metaswap_entry()
{
	lock_acquire(&swap_lock);
	int i;
	for(i=0; i<swaptable_size; i++){
		if(!swaptable[i].isfilled) {
			lock_release(&swap_lock);
			return &swaptable[i];
		}
	}
	lock_release(&swap_lock);
	PANIC("No more swap space!!!!");
	return NULL;
}

int 
move_into_swap(void* page, enum load_instruction instruction) {
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
	lock_acquire(&swap_lock);
	new_swap_entry->isfilled = true;
	new_swap_entry->instructions_for_pageheld = instruction;
	lock_release(&swap_lock);
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

enum load_instruction
free_metaswap_entry(int index){
	lock_acquire(&swap_lock);
	struct metaswap_entry* swap_entry2free = get_metaswap_entry_byindex(index);
	ASSERT(swap_entry2free->isfilled);
	enum load_instruction saved_instruction = swap_entry2free->instructions_for_pageheld;
	swap_entry2free->isfilled = false;
	swap_entry2free->instructions_for_pageheld = USELESS;
	lock_release(&swap_lock);
	return saved_instruction;
}

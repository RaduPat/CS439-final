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
move_into_swap(void* page, enum load_instruction instruction, bool isdirty) {
	if(page == NULL)
		PANIC("Page in move_into_swap == NULL");
	struct metaswap_entry* new_swap_entry = next_empty_metaswap_entry();
	if(new_swap_entry==NULL)
		PANIC("no empty swap entries available");

	char * buffer = (char*) page;
	//char * buffer = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
	int i;
	if (instruction == STACK)
	{
		/*printf("@@@@@@@@@@@@@ in move_into_swap()\n");
		for (i = 0; i < 100; ++i)
		{
			printf("# (%x)", ((char *)page)[4096 - i]);
		}*/
	}
/*		if (instruction == STACK)
		{
			printf("||||||||||| %x\n", buffer);
		}
*/	for(i = 0; i < 8; i++)
	{
		//printf("++++++++++++ %d\n", 8 * new_swap_entry->swap_index + i);
		block_write(swap_block, 8 * new_swap_entry->swap_index + i, buffer);
		buffer += BLOCK_SECTOR_SIZE;
	}
	//printf("(%d %d)\n", new_swap_entry->swap_index, instruction);
	if (instruction == STACK){
	/*printf("\n &&&&&&& just printing for stack\n");
		char * temp = (char *) page;
		for (i = 0; i < 4096; ++i)
		{
			printf("# (%x) %x | ", temp[i], temp+i);
		}*/
	}
	lock_acquire(&swap_lock);
	new_swap_entry->isfilled = true;
	new_swap_entry->instructions_for_pageheld = instruction;
	new_swap_entry->isdirty = isdirty;
	lock_release(&swap_lock);
	return new_swap_entry->swap_index;
}

void
read_from_swap(int index, void *page)
{
	//printf("(((((%d %x)))))\n", index, page);
	ASSERT(index != -1);
	if(page == NULL)
		PANIC("Page in read_from_swap == NULL");
	char * buffer = (char*) page;
	int i;
	/*printf("$$$$$$$$$$ Start\n");
	for (i = 0; i < 100; ++i)
	{
		printf("# (%x)",  buffer[4096 - i]);
	}*/
	//char tempbuffer[512];
	for(i = 0; i < 8; i++)
	{
		/*printf("+++++=======+++++++::::::: buffer address: %x\n", buffer);
		printf("------------------ %d\n", 8 * index + i);*/
		/*if (i == 7)
		{
			block_read(swap_block, 8*index + i, tempbuffer);
		}// the data is stored in the reverse order? Index number 7 is appearing from (0,512) in buffer[]*/
		block_read(swap_block, 8 * index + i, buffer);//block_read into the buffer not working?
		buffer += BLOCK_SECTOR_SIZE;
	}
	/*printf("$$$$$$$$$$ Start\n");
	for (i = 0; i < 512; ++i)
	{
		printf("-(%x)- ", tempbuffer[i]);
	}
	for (i = 3584; i < 4096; ++i)
	{
		printf("# (%x)", buffer[i]);
	}
	printf("\n\n\n");*/
}

/* Two putative problems:

1.	Data not being written from the swap block to the page (void* buffer). Only the last swap sector is being written
2.	Data being written in the reverse order. The last swap sector is copied to the first 512 bytes in page (void* buffer)
2.	Stack data not being written to the swap space properly. */

struct metaswap_entry*
free_metaswap_entry(int index){
	ASSERT(index != -1);
	lock_acquire(&swap_lock);
	struct metaswap_entry* swap_entry2free = get_metaswap_entry_byindex(index);
	if(swap_entry2free == NULL)
		PANIC("swap_entry2free == NULL");
	ASSERT(swap_entry2free->isfilled);
	swap_entry2free->isfilled = false;
	swap_entry2free->instructions_for_pageheld = USELESS;
	lock_release(&swap_lock);
	return swap_entry2free;
}

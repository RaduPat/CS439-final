#include "frametable.h"
#include "threads/palloc.h"
#include "vm/swaptable.h"
#include <inttypes.h>
#include "threads/vaddr.h"


int num_user_frames;

struct metaframe * frametable; 

struct lock ft_lock;

uint32_t clock_hand = 0;

//debug info

int num_pages_assigned = 0;

//End

void
init_frametable(uint32_t init_ram_pages)
{
	lock_init(&ft_lock);

	/* Free memory starts at 1 MB and runs to the end of RAM. */
  uint8_t *free_start = ptov (1024 * 1024);
  uint8_t *free_end = ptov (init_ram_pages * PGSIZE);
  size_t free_pages = (free_end - free_start) / PGSIZE;
  size_t user_pages = free_pages / 2;

	num_user_frames = user_pages - 1;
	frametable = calloc(sizeof(struct metaframe), num_user_frames);
	int i;
	for(i = 0; i < num_user_frames; i++) 
	{
		frametable[i].page = NULL;
		frametable[i].isfilled = false;
	}
}

int num_free_frames(){
	int sum = 0;
	int i;
	for (i = 0; i < num_user_frames; ++i)
	{
		if (frametable[i].isfilled)
		{
			sum++;
		}
	}
	return sum;
}

struct metaframe* 
get_metaframe_bypage(void* page) {
	if(page == NULL)
		PANIC("Page in get metaframe by page == NULL");
	int i;
	for(i = 0; i < num_user_frames; i++){
		if(frametable[i].page == page){
			return &frametable[i];
		}
	}
	return NULL;
}

struct metaframe* 
next_empty_frame(){
	lock_acquire(&ft_lock);
	int i;
	for(i=0; i<num_user_frames; i++){
		if(!frametable[i].isfilled)
			 {
			 		lock_release(&ft_lock);
			 		printf("********** not doing eviction\n");
			 		printf("kpage_address: %x\n", frametable[i].page);
			 		printf("owner: %x\n", frametable[i].owner);
			 		printf("metaframe address: %x\n", &frametable[i]);
			 		return &frametable[i];
			 }
	}
	lock_release(&ft_lock);

	return evict_page();
}

void* 
assign_page(){
	struct metaframe* new_frame = next_empty_frame();
	if(new_frame==NULL)
		PANIC("no empty frames available");
	void* new_page = palloc_get_page(PAL_USER | PAL_ZERO);
	//printf("*********** new_page: %x\n", new_page);
	if(new_page==NULL){
		printf("######### size of frametable: %d/%d\n", num_free_frames(), num_user_frames);
		//PANIC("no available pages");
	}
	lock_acquire(&ft_lock);
	new_frame->page = new_page;
	new_frame->isfilled = true;
	num_pages_assigned++;
	new_frame->owner = thread_current();
	printf("------------ frame owner in assign_page: %x\n", new_frame->owner);
	printf("------------ frame kpage_address in assign_page: %x\n", new_frame->page);
	printf("------------ metaframe address in assign_page: %x\n", new_frame);
	printf("=========== Number of pages assigned: %d\n", num_pages_assigned);
	lock_release(&ft_lock);

	return new_page;
}

void
free_frame(void* page){
	if(page == NULL)
		PANIC("Page in free frame == NULL");
	lock_acquire(&ft_lock);
	struct metaframe* frame2free = get_metaframe_bypage(page);
	if(frame2free == NULL)
		PANIC("frame2free == NULL");
	frame2free->page = NULL;
	frame2free->owner = NULL;
	frame2free->isfilled = false;
	lock_release(&ft_lock);
}

// Implement page eviction using the clock algorithm
static struct metaframe*
evict_page()
{

	struct spinfo * current_spinfo = find_spinfo_by_kpage((&(frametable[clock_hand].owner)->spage_table), frametable[clock_hand].page);
	if (current_spinfo == NULL)
	{
		printf("################## null current_spinfo!\n");
		printf("clock_hand: %d\n", clock_hand);
		printf("frametable[clock_hand].page: %x\n", frametable[clock_hand].page);
		printf("frametable[clock_hand].owner: %x\n", frametable[clock_hand].owner);
		printf("thread name: %s\n", frametable[clock_hand].owner->name);
		PANIC("CURRENT_SPINFO == NULL");
	}
	void * current_page = current_spinfo->upage_address;
	if(current_page == NULL)
		PANIC("current_page == NULL");
	while(pagedir_is_accessed((frametable[clock_hand].owner)->pagedir, current_page)){
		pagedir_set_accessed((frametable[clock_hand].owner)->pagedir, current_page, false);
		clock_hand++;
		if (clock_hand >= num_user_frames)
		{
			clock_hand = 0;// move the clock_hand back to the start
		}
		current_spinfo = find_spinfo_by_kpage((&(frametable[clock_hand].owner)->spage_table), frametable[clock_hand].page);
		if(current_spinfo == NULL)
			PANIC("CURRENT_SPINFO LOOP == NULL");
		current_page = current_spinfo->upage_address;
	}

	if (frametable[clock_hand].isfilled)
	{
		printf("########## isfilled : true\n");
	}
	else{
		printf("########## isfilled : false\n");
	}
	printf("meta frame address: %x\n", &frametable[clock_hand]);
	printf("frame kpage address: %x\n", frametable[clock_hand].page);
	printf("frame owner address: %x\n", frametable[clock_hand].owner);
	printf("upage that's getting thrown out: %x\n", current_page);

	// remove page from owner's page table, and write it to swap. check to see if the page is for stack or dirty
	if(current_spinfo->instructions == STACK || pagedir_is_dirty((frametable[clock_hand].owner)->pagedir, current_page))
	{
		current_spinfo->index_into_swap = move_into_swap(frametable[clock_hand].page, current_spinfo->instructions);
			//printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ swapped out a stack page\n");
		current_spinfo->instructions = SWAP;
	}
	
	pagedir_clear_page(frametable[clock_hand].owner->pagedir, current_page);

	// evict the chosen page from the frame
	current_spinfo->kpage_address = NULL;//setting it to null just here would suffice because in all other locations the spinfo is freed.
	palloc_free_page(frametable[clock_hand].page);//freeing the page because it becomes garbage after this
		if(current_spinfo->instructions == SWAP){
			printf("$$$$$$$$$$ Start\n");
			int i;
			printf("@@@@@@@@@@@@@ in evict_page()\n");
			for (i = 0; i < 100; ++i)
			{
				printf("# (%x)", ((char *)frametable[clock_hand].page)[4096 - i]);
			}
			PANIC("stack evict_page()");
		}
	free_frame(frametable[clock_hand].page);

	return &frametable[clock_hand];
}


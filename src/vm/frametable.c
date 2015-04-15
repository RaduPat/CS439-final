#include "frametable.h"
#include "threads/palloc.h"
#include "vm/swaptable.h"
#include <inttypes.h>


int num_user_frames;

struct metaframe * frametable; 

struct lock ft_lock;

uint32_t clock_hand = 0;

void
init_frametable(uint32_t init_ram_pages)
{
	lock_init(&ft_lock);
	num_user_frames = (int) init_ram_pages/2;
	frametable = calloc(sizeof(struct metaframe), num_user_frames);
	int i;
	for(i = 0; i < num_user_frames; i++) 
	{
		frametable[i].page = NULL;
		frametable[i].isfilled = false;
	}
}

struct metaframe* 
get_metaframe_bypage(void* page){
	int i;
	for(i = 0; i < num_user_frames; i++){
		if(frametable[i].page == page){
			return &frametable[i];
		}
	}
	return NULL;
}

static struct metaframe* 
next_empty_frame(){
	lock_acquire(&ft_lock);
	int i;
	for(i=0; i<num_user_frames; i++){
		if(!frametable[i].isfilled)
			return &frametable[i];
	}
	lock_release(&ft_lock);

	return evict_page();
}

void* 
assign_page(){
	void* new_page = palloc_get_page(PAL_USER | PAL_ZERO);
	if(new_page==NULL)
		PANIC("no available pages");
	struct metaframe* new_frame = next_empty_frame();
	if(new_frame==NULL)
		PANIC("no empty frames available");
	lock_acquire(&ft_lock);
	new_frame->page = new_page;
	new_frame->isfilled = true;
	new_frame->owner = thread_current();
	lock_release(&ft_lock);

	return new_page;
}

void
free_frame(void* page){
	lock_acquire(&ft_lock);
	struct metaframe* frame2free = get_metaframe_bypage(page);
	ASSERT(frame2free != NULL);
	frame2free->owner = NULL;
	frame2free->isfilled = false;
	lock_release(&ft_lock);
}

// Implement page eviction using the clock algorithm
static struct metaframe*
evict_page()
{

	struct spinfo * current_spinfo = find_spinfo((frametable[clock_hand].owner)->spage_table, frametable[clock_hand].page);
	void * current_page = current_spinfo->upage_address;
	while(pagedir_is_accessed((frametable[clock_hand].owner)->pagedir, current_page)){
		pagedir_set_accessed((frametable[clock_hand].owner)->pagedir, current_page, false);
		clock_hand++;
		if (clock_hand >= user_frames)
		{
			clock_hand = 0;// move the clock_hand back to the start
		}
		current_spinfo = find_spinfo((frametable[clock_hand].owner)->spage_table, frametable[clock_hand].page);
		current_page = current_spinfo->upage_address;
	}

	// remove page from owner's page table, and write it to swap
	current_spinfo->index_into_swap = move_into_swap(frametable[clock_hand].page);
	set_type_of_page_swapped_in(current_spinfo->index_into_swap, current_spinfo->instructions);
	pagedir_clear_page(frametable[clock_hand].owner->pagedir, current_page);

	// evict the chosen page from the frame
	find_spinfo((frametable[clock_hand].owner)->spage_table, frametable[clock_hand].page)->instructions = SWAP;
	free_frame(frametable[clock_hand].page);

	return &frametable[clock_hand];
}


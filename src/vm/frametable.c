#include "frametable.h"
#include "spagetable.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include <inttypes.h>


extern uint32_t init_ram_pages;
const uint32_t user_frames = 1024; //init_ram_pages/2; // note that this variable is used in evict page

struct metaframe frametable[1024]; 

struct lock ft_lock;

uint32_t clock_hand = 0;

void
frametable_init()
{
	lock_init(&ft_lock);
}

struct metaframe* 
get_metaframe_byindex(int index){
	return &frametable[index];
}

struct metaframe* 
get_metaframe_bypage(void* page){
	int i;
	for(i = 0; i < user_frames; i++){
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
	for(i=0; i<user_frames; i++){
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
	lock_release(&ft_lock);

	return new_page;
}

void
free_frame(void* page){
	lock_acquire(&ft_lock);
	struct metaframe* frame2free = get_metaframe_bypage(page);
	ASSERT(frame2free != NULL);

	frame2free->isfilled = false;
	lock_release(&ft_lock);
}

static struct spinfo*
get_spinfo_by_frameindex(int frame_index)//what if owner of page at thread_index isn't thread_current
{
	struct spinfo * target_spinfo = find_spinfo_by_frame(&thread_current()->spage_table, frametable[i].page);
	return target_spinfo;
}

// Implement page eviction using the clock algorithm
static struct metaframe*
evict_page()
{

	void * current_page = get_spinfo_by_frameindex(clock_hand)->upage_address;
	while(pagedir_is_accessed(thread_current()->pagedir, current_page)){
		pagedir_set_accessed(thread_current()->pagedir, current_page, false);
		clock_hand++;
		if (clock_hand >= user_frames)
		{
			clock_hand = 0;// move the clock_hand back to the start
		}
		current_page = get_upage_address(clock_hand)->upage_address;
	}

	// evict the chosen page from the frame
	get_spinfo_by_frameindex(clock_hand)->instructions = SWAP;
	free_frame(frametable[clock_hand].page);
	return &frametable[clock_hand];
}

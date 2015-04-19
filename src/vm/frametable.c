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

struct metaframe* 
get_metaframe_bypage(void* page) {// not locking because its calling function already locks
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
	int i;
	lock_acquire(&ft_lock);
	for(i=0; i<num_user_frames; i++){
		if(!frametable[i].isfilled)
			 {
			 		lock_release(&ft_lock);
			 		return &frametable[i];
			 }
	}
	lock_release(&ft_lock);

	return evict_page();
}

void* 
assign_page(){
	struct metaframe* new_frame = next_empty_frame();
	void* new_page = palloc_get_page(PAL_USER | PAL_ZERO);
	lock_acquire(&ft_lock);
	new_frame->page = new_page;
	new_frame->isfilled = true;
	num_pages_assigned++;
	new_frame->owner = thread_current();
	lock_release(&ft_lock);

	return new_page;
}

void
free_frame(void* page){
	struct metaframe* frame2free = get_metaframe_bypage(page);
	frame2free->page = NULL;
	frame2free->owner = NULL;
	frame2free->isfilled = false;
}

// Implement page eviction using the clock algorithm
static struct metaframe*
evict_page()// change to do while loop because owner_of_frame changes every time, so lock changes
{
	struct thread * owner_of_frame;
	uint8_t * current_kpage;
	struct spinfo * current_spinfo;
	void * current_page;
	bool is_accessed_bit_set;
	lock_acquire(&ft_lock);

	do{
		clock_hand++;
		if (clock_hand >= num_user_frames)
		{
			clock_hand = 0;// move the clock_hand back to the start
		}

		owner_of_frame = frametable[clock_hand].owner;
		lock_acquire(&owner_of_frame->spage_lock);
		current_kpage = frametable[clock_hand].page;
		current_spinfo = find_spinfo_by_kpage(&owner_of_frame->spage_table, current_kpage);
		current_page = current_spinfo->upage_address;
		is_accessed_bit_set = pagedir_is_accessed(owner_of_frame->pagedir, current_page);
		pagedir_set_accessed(owner_of_frame->pagedir, current_page, false);

		if (is_accessed_bit_set)
		{
			lock_release(&owner_of_frame->spage_lock);
		}
	}
	while(is_accessed_bit_set);

	// remove page from owner's page table, and write it to swap. check to see if the page is for stack or dirty
	bool page_isdirty = pagedir_is_dirty(owner_of_frame->pagedir, current_page);
	if(current_spinfo->instructions == STACK || page_isdirty)
	{
		current_spinfo->index_into_swap = move_into_swap(current_kpage, current_spinfo->instructions, page_isdirty);
		current_spinfo->instructions = SWAP;
	}
	
	pagedir_clear_page(owner_of_frame->pagedir, current_page);

	// evict the chosen page from the frame
	current_spinfo->kpage_address = NULL;//setting it to null just here would suffice because in all other locations the spinfo is freed.
	palloc_free_page(current_kpage);//freeing the page because it becomes garbage after this
	free_frame(current_kpage);
	lock_release(&ft_lock);
	lock_release(&owner_of_frame->spage_lock);

	//synchronizing the spage_table in exception.c
	/*lock_acquire(&ft_lock);
	struct thread * owner_of_frame = frametable[clock_hand].owner;
	lock_acquire(&owner_of_frame->spage_lock);
	uint8_t * current_kpage = frametable[clock_hand].page;
	struct spinfo * current_spinfo = find_spinfo_by_kpage(&owner_of_frame->spage_table, current_kpage);
	void * current_page = current_spinfo->upage_address;
	while(pagedir_is_accessed(owner_of_frame->pagedir, current_page)){

		pagedir_set_accessed(owner_of_frame->pagedir, current_page, false);
		clock_hand++;
		if (clock_hand >= num_user_frames)
		{
			clock_hand = 0;// move the clock_hand back to the start
		}
	    owner_of_frame = frametable[clock_hand].owner;
		current_kpage = frametable[clock_hand].page;
		ASSERT(current_kpage != NULL);
		current_spinfo = find_spinfo_by_kpage(&owner_of_frame->spage_table, current_kpage);
		current_page = current_spinfo->upage_address;
	}

	// remove page from owner's page table, and write it to swap. check to see if the page is for stack or dirty
	bool page_isdirty = pagedir_is_dirty(owner_of_frame->pagedir, current_page);
	if(current_spinfo->instructions == STACK || page_isdirty)
	{
		current_spinfo->index_into_swap = move_into_swap(current_kpage, current_spinfo->instructions, page_isdirty);
		current_spinfo->instructions = SWAP;
	}
	
	pagedir_clear_page(owner_of_frame->pagedir, current_page);

	// evict the chosen page from the frame
	current_spinfo->kpage_address = NULL;//setting it to null just here would suffice because in all other locations the spinfo is freed.
	lock_release(&ft_lock);
	lock_release(&owner_of_frame->spage_lock);
	palloc_free_page(current_kpage);//freeing the page because it becomes garbage after this
	free_frame(current_kpage);*/

	return &frametable[clock_hand];
}


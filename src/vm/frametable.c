#include "frametable.h"
#include "threads/palloc.h"
#include "vm/swaptable.h"
#include <inttypes.h>
#include "threads/vaddr.h"

int num_user_frames;
struct metaframe * frametable; 

uint32_t clock_hand = 0;

int num_pages_assigned = 0;

/* Andrew and Radu drove here */
void
init_frametable(uint32_t init_ram_pages)
{
	/* Free memory starts at 1 MB and runs to the end of RAM. */
  uint8_t *free_start = ptov (1024 * 1024);
  uint8_t *free_end = ptov (init_ram_pages * PGSIZE);
  size_t free_pages = (free_end - free_start) / PGSIZE;
  size_t user_pages = free_pages / 2;

	num_user_frames = user_pages - 1;
	frametable = calloc (sizeof(struct metaframe), num_user_frames);
	int i;
	for(i = 0; i < num_user_frames; i++) 
		{
			frametable[i].page = NULL;
			frametable[i].isfilled = false;
		}
}

struct metaframe* 
get_metaframe_bypage(void* page) 
{// not locking because its calling function already locks
	if(page == NULL)
		PANIC("Page in get metaframe by page == NULL");
	int i;
	for(i = 0; i < num_user_frames; i++)
		{
			if(frametable[i].page == page)
				{
					return &frametable[i];
				}
		}
	return NULL;
}

struct metaframe* 
next_empty_frame()
{
	int i;
	for(i=0; i<num_user_frames; i++)
	{
		if(!frametable[i].isfilled)
		 {
		 		return &frametable[i];
		 }
	}

	return evict_page();
}

void* 
assign_page()
{
	struct metaframe* new_frame = next_empty_frame ();
	void* new_page = palloc_get_page (PAL_USER | PAL_ZERO);
	new_frame->page = new_page;
	new_frame->isfilled = true;
	num_pages_assigned++;
	new_frame->owner = thread_current ();

	return new_page;
}

void
free_frame (void* page)
{
	struct metaframe* frame2free = get_metaframe_bypage (page);
	frame2free->page = NULL;
	frame2free->owner = NULL;
	frame2free->isfilled = false;
}

// Implement page eviction using the clock algorithm
static struct metaframe*
evict_page ()// change to do while loop because owner_of_frame changes every time, so lock changes
{
	struct thread * owner_of_frame;
	uint8_t * current_kpage;
	struct spinfo * current_spinfo;
	void * current_page;
	bool is_accessed_bit_set;

	do
		{
			clock_hand++;
			if (clock_hand >= num_user_frames)
				clock_hand = 0;// move the clock_hand back to the start
			
			owner_of_frame = frametable[clock_hand].owner;
			current_kpage = frametable[clock_hand].page;
			current_spinfo = find_spinfo_by_kpage (&owner_of_frame->spage_table, current_kpage);
			current_page = current_spinfo->upage_address;
			is_accessed_bit_set = pagedir_is_accessed (owner_of_frame->pagedir, current_page);
			pagedir_set_accessed (owner_of_frame->pagedir, current_page, false);
		}
	while(is_accessed_bit_set);

	// remove page from owner's page table, and write it to swap. check to see if the page is for stack or dirty
	bool page_isdirty = pagedir_is_dirty (owner_of_frame->pagedir, current_page);
	if(current_spinfo->instructions == STACK || page_isdirty)
		{
			current_spinfo->index_into_swap = move_into_swap (current_kpage, current_spinfo->instructions, page_isdirty);
			current_spinfo->instructions = SWAP;
		}
	
	pagedir_clear_page (owner_of_frame->pagedir, current_page);

	// evict the chosen page from the frame
	current_spinfo->kpage_address = NULL;//setting it to null just here would suffice because in all other locations the spinfo is freed.
	palloc_free_page (current_kpage);//freeing the page because it becomes garbage after this
	free_frame (current_kpage);

	return &frametable[clock_hand];
}


#include "frametable.h"
#include "threads/palloc.h"
#include <inttypes.h>


int num_user_frames;

struct metaframe * frametable; 

void
init_frametable(uint32_t init_ram_pages)
{
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
get_metaframe_byindex(int index)
{
	return &frametable[index];
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

struct metaframe* 
next_empty_frame(){
	int i;
	for(i=0; i<num_user_frames; i++){
		if(!frametable[i].isfilled)
			return &frametable[i];
	}
	return NULL;
}

void* 
assign_page(){
	void* new_page = palloc_get_page(PAL_USER | PAL_ZERO);
	if(new_page==NULL)
		PANIC("no available pages");
	struct metaframe* new_frame = next_empty_frame();
	if(new_frame==NULL)
		PANIC("no empty frames available");
	new_frame->page = new_page;
	new_frame->isfilled = true;
	new_frame->owner = thread_current();

	return new_page;
}

void
free_frame(void* page){
	struct metaframe* frame2free = get_metaframe_bypage(page);
	ASSERT(frame2free != NULL);

	frame2free->isfilled = false;
}


#ifndef VM_FRAMETABLE_H
#define VM_FRAMETABLE_H

#include <stdbool.h>
#include <debug.h>
#include <stdint.h>
#include "threads/thread.h"

//meta data for a frame in the frame table 
struct metaframe
{
	bool isfilled;
	void *page;	
	struct thread * owner;
};
//dynamically allocate memory for the frame table
void init_frametable(uint32_t init_ram_pages);
//get a metaframe in the table by page
struct metaframe* get_metaframe_bypage(void* page);
//get the next available metaframe within the frame table
//static struct metaframe* next_empty_frame(void);
//assign a page for the frame
void* assign_page(void);
//free up a frame
void free_frame(void* page);
//evict a page
static struct metaframe*evict_page(void);


#endif /* vm/frametable.h */

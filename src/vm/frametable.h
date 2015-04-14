#ifndef VM_FRAMETABLE_H
#define VM_FRAMETABLE_H

#include <stdbool.h>
#include <debug.h>

//meta data for a frame in the frame table 
struct metaframe
{
	bool isfilled;
	void *page;				// Kernel page associated with a frame
};

//get a metaframe in the table by page
struct metaframe* get_metaframe_bypage(void* page);
//get the metaframe at an index within the frame table
struct metaframe* get_metaframe_byindex(int index);
//get the next available metaframe within the frame table
struct metaframe* next_empty_frame(void);
//assign a page for the frame
void* assign_page(void);
//free up a frame
void free_frame(void* page);


#endif /* vm/frametable.h */

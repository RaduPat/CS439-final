#include "spagetable.h"

struct spinfo * find_spinfo (struct list * info_list, uint8_t * page){
	struct list_elem * e;
  for (e = list_begin (info_list);
         e != list_end (info_list); e = list_next (e))
    {
      struct spinfo *spage_info = list_entry (e, struct spinfo, sptable_elem);
      if (spage_info->upage_address == page)
      {
      	return spage_info;
      }
    }
    return NULL;
}
#include "spagetable.h"

struct spinfo * find_spinfo (struct list * info_list, uint8_t * page){
  if(page == NULL) 
  {
    debug_backtrace();
      PANIC("Page in find_spinfo == NULL");
  }
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

struct spinfo * find_spinfo_by_kpage (struct list * info_list, uint8_t * kpage){
  if(kpage == NULL)
    PANIC("kpage in find_spinfy_by_kpage == NULL");
  struct list_elem * e;
  for (e = list_begin (info_list);
         e != list_end (info_list); e = list_next (e))
    {
      struct spinfo *spage_info = list_entry (e, struct spinfo, sptable_elem);
      //printf("^^^^^^^ kpage_address: %x, target: %x\n", spage_info->kpage_address, kpage);
      if (spage_info->kpage_address == kpage)
      {
        return spage_info;
      }
    }
    return NULL;
}

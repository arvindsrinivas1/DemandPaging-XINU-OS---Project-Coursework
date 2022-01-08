#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {
// Does not have anything to do with a pid. Generic. So we init bs_pids and npgaes with -1

  /* requests a new mapping of npages with ID map_id */
   int i;
   if(bsm_tab[bs_id].bs_status == BSM_MAPPED){
	// Already mapped BS, so just return
	return bsm_tab[bs_id].bs_size;
   }
   else{
	// We have to create the BS with npages size
	bsm_tab[bs_id].bs_status = BSM_MAPPED;
	bsm_tab[bs_id].bs_vheap_check = NOT_VHEAP;
	//initialize bs_pids
	init_bsm_tab_pids(bs_id);
	//initialize bs_npages
	init_bsm_tab_npages(bs_id);
	bsm_tab[bs_id].bs_sem = -1;
	bsm_tab[bs_id].bs_size = npages; 
   }

//    kprintf("To be implemented!\n");
    return npages;

}



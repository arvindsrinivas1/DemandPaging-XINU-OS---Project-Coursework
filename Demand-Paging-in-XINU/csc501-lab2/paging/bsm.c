/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
//bs_map_t bsm_tab[8];

void reset_bsm_entry(int i){
                bsm_tab[i].bs_status = BSM_UNMAPPED;
		bsm_tab[i].bs_vheap_check = NOT_VHEAP;
		//bsm_tab[i].bs_pid = -1;
                //bsm_tab[i].bs_vpno = -1;
		init_bsm_tab_pids(i);
                init_bsm_tab_npages(i);
                bsm_tab[i].bs_sem = 0;
		bsm_tab[i].bs_size = 0;
}



SYSCALL init_bsm()
{
	//NOTE: 8 BS. Hence bsm_tab is initialized only for indexes 0-7
	//NOTE: Present in the kernel data segment
	int i = 0
;
	for(i = 0; i <= 7; i++){
		reset_bsm_entry(i);
	}
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	//NOTE: A free entry from bsm_tab means a backing store that has not been mapped(bs_status = 0)
	int i = 0;
	int found = 0;
	for(i = 0; i <= 7; i++){
		if(bsm_tab[i].bs_status == BSM_UNMAPPED){
			*avail = i;
			found = 1;
			break;
		}			
	}
	//SYCALL is just int. We return i(bs_t id) if an unmapped bsm_enty(unmapped backing store) is present. Else we return 0 (failure);
	if(found == 1){
		return i;
	}
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
		reset_bsm_entry(i);
	//Note: Will reset to initial values
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	volatile int vpage = vaddr/NBPG;
	volatile int i,j;
	// Since we are doing this for both vheap and otherwise
	// make sure you have the values filled even in vheap BS
	for(i = 0; i < 8 ; i++){
		if(bsm_tab[i].bs_status == BSM_UNMAPPED){
			continue;
		}
		for(j = 0; j< MAX_BS_PAGES; j++){
			//go through bs_pids
			if(bsm_tab[i].bs_pids[j] == pid){
				if(bsm_tab[i].bs_npages[j] == vpage){
					*store = i;
					*pageth = j;
					//return which bs and in that which page it is in 
					return OK;
				}
			}
		}// go through pids and npages to see where it is.
	}// go through every store.
	return SYSERR; // if we dont find the page & bs for the given pid and vaddr
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	int i, overflow_check, filled_size;
	overflow_check = check_bs_overflow(source, npages);
	if(overflow_check == SYSERR){
		return SYSERR;
	}
	else{
		if(bsm_tab[source].bs_status ==  BSM_UNMAPPED){
			bsm_tab[source].bs_status =  BSM_MAPPED;
		}
        	while(bsm_tab[source].bs_pids[i] != -1 && i < bsm_tab[source].bs_size){
                	i++;
        	}
		filled_size = i;
		//for(; i < filled_size + npages; i++){
		// currently assuming no holes in the backing store.
		for(i = 0 ; i < npages; i++){
			bsm_tab[source].bs_pids[filled_size + i] = pid;
			bsm_tab[source].bs_npages[filled_size + i] = vpno + i;
		}
		//changetest1: Not adding new pages to BS. Just mapping to existing BS pages.
		//bsm_tab[source].bs_size += npages;
		return OK;
	}
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	int bs = 0, bs_page = 0;
	int store = 0;
	int pageth = 0;
	int vaddr = vpno * NBPG; // doing this so that we can use bsm_lookup;
	int frame_lookup_result = 0, bs_lookup_result = 0;
	int swap_free_frame = -1;
	char *content_frame;

	bs_lookup_result = bsm_lookup(pid, vaddr, &store, &pageth);
	if (bs_lookup_result == OK){
		//get_frm(&swap_free_frame);

		//set frame values
		//frm_tab[swap_free_frame].fr_status = FRM_MAPPED;
		//frm_tab[swap_free_frame].fr_pid = pid;
		//frm_tab[swap_free_frame].fr_vpno = bsm_tab[store].bs_npages[pageth];
		//frm_tab[swap_free_frame].fr_refcnt = 1;
		//frm_tab[swap_free_frame].fr_type = FR_PAGE;
		//frm_tab[swap_free_frame].fr_dirty = 0;

		//change to absolute frame number
		//swap_free_frame = FRAME0 + swap_free_frame;

		//Write out BS page to free frame
		//content_frame = (char *)(swap_free_frame * NBPG);		
		//read_bs(content_frame, store, pageth); //write bs page into content_frame

		//Empty BS page. There is an empty space in BS now. How to handle it?
		//Assuming the page has already beed written out.

//		bsm_lookup(pid, vaddr, &store, &pageth);


		int i = 0;

		frame_lookup_result = frame_lookup_for_vpno(vpno);
		if(frame_lookup_result == SYSERR){
			// NO page for that vpno in frames
		}
		else{
//			if(frm_tab[frame_lookup_result].fr_dirty == 1){
				// BS lookup success, framelookup success, frame is dirty. So write it back to BS.
				content_frame = (char *)((FRAME0 + frame_lookup_result)*NBPG);
				write_bs(content_frame, store, pageth);
//			}
			//Reset frame values anyway
			frm_tab[frame_lookup_result].fr_status = FRM_UNMAPPED;
			frm_tab[frame_lookup_result].fr_pid = -1;
			frm_tab[frame_lookup_result].fr_vpno = -1;
			frm_tab[frame_lookup_result].fr_refcnt = 0;
			frm_tab[frame_lookup_result].fr_dirty = 0;
		}


		bsm_tab[store].bs_pids[pageth] = -1;
		bsm_tab[store].bs_npages[pageth] = -1;

	}
}



/* Check npages < 256 */
SYSCALL check_bs_overflow(bsd_t source, int npages){
//for vheap just use npages
//for non-vheap bs, loop from npages[i] to see how many -1 are there
	int i = 0;
	while(bsm_tab[source].bs_pids[i] != -1 && i < bsm_tab[source].bs_size){
		i++;
	}
	//Again, assuming no holes in BS.
	if( (i) + npages > bsm_tab[source].bs_size ){
		return SYSERR;
	}
	return OK;
}

SYSCALL init_bsm_tab_npages(int entry){
	int i;
	for(i = 0; i < MAX_BS_PAGES; i++){
		bsm_tab[entry].bs_npages[i] = -1;
	}
	return OK;
}

SYSCALL init_bsm_tab_pids(int entry){
	int i;
	for(i = 0; i<MAX_BS_PAGES; i++){
		bsm_tab[entry].bs_pids[i] = -1;
	}
	return OK;
}

SYSCALL set_bsm_tab_pids(int entry, int pid){
	int i;
	for(i=0; i<bsm_tab[entry].bs_size; i++){
		bsm_tab[entry].bs_pids[i] = pid;	
	}
	return OK;
}

SYSCALL set_bsm_tab_npages(int entry, int vpgno){
	int i;
	for(i=0; i<bsm_tab[entry].bs_size; i++){
		bsm_tab[entry].bs_npages[i] = vpgno;
		vpgno++;
	}
	return OK;
}

SYSCALL get_first_page_bs(int entry){
	//1024 - 2047 are the frames for actual storage. 2048 - 4095 is for Backing Stores
	return (BS_STARTING_PAGE + (MAX_BS_PAGES * (entry)) );
}

SYSCALL get_first_addr_bs(int entry){
	return (BACKING_STORE_BASE + (BACKING_STORE_UNIT_SIZE * entry));
}

SYSCALL swap_out_page_reset(int store, int pageth){
	if(bsm_tab[store].bs_vheap_check == IS_VHEAP){
		bsm_tab[store].bs_pids[pageth] = -1;
		bsm_tab[store].bs_npages[pageth] = -1;
	}
	else{
		bsm_tab[store].bs_pids[pageth] = -1;
		bsm_tab[store].bs_npages[pageth] = -1;
	}
}


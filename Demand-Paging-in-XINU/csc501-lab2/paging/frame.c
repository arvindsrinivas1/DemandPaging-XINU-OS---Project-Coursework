/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */

//fr_map_t frm_tab[NFRAMES];

SYSCALL init_frm()
{
  //kprintf("To be implemented!\n");
  int i;
  for(i = 0; i <= NFRAMES - 1; i++){
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_pid = -1;
    frm_tab[i].fr_vpno = -1;
    frm_tab[i].fr_refcnt = -1;
    frm_tab[i].fr_type = FR_PAGE;
    frm_tab[i].fr_dirty = 0;
    frm_tab[i].age = 0;
  } 
 
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
//  kprintf("To be implemented!\n");
	int free_frame = 0, replace_frame, pt_frame; 
	int vp, pid, i = 0;
	unsigned long start_virt_addr;
	virt_addr_t *start_virt_addr_struct;
	pt_t *pte;
	pd_t *pde;
	unsigned long pdbr;
	int bsm_lookup_result, store, pageth;
	char * content_frame;

	for(i = 0; i < NFRAMES; i++){
		if(frm_tab[i].fr_status == FRM_UNMAPPED){
			*avail = i;
			free_frame = i;
			break;
		}
	}
	if(free_frame != 0 ){
		// There was a free frame available. Replacement not needed
		//circular_queue[cq_insert_ptr].frame_id = free_frame;
/*
		if(cq_insert_ptr == NFRAMES - 1){
			circular_queue[cq_insert_ptr].frame_id = free_frame;
			circular_queue[cq_insert_ptr].next = 0; // cq_head does not have to be zero always? 
			//cq_insert_ptr = 0; // ?????????? 
		}
		else{
			circular_queue[cq_insert_ptr].frame_id = free_frame;
			circular_queue[cq_insert_ptr].next = cq_insert_ptr + 1;
			++cq_insert_ptr;
		}
*/
		//cq_rear = free_frame;
		return free_frame;
	}
	else{
		//Implement Page replacement policy	
		//kprintf("Page replacement ?????????\n");

		replace_frame = page_replacement_policy();	
		if(get_debugger() == 1){
			kprintf("Frame %d is replaced\n", replace_frame + FRAME0);
		}
		vp = frm_tab[replace_frame].fr_vpno;		
		pid = frm_tab[replace_frame].fr_pid;
		pdbr = proctab[pid].pdbr;
		start_virt_addr = vp * NBPG;
		start_virt_addr_struct = (virt_addr_t*)(&start_virt_addr);

		pde = (pd_t*)(proctab[pid].pdbr + (start_virt_addr_struct->pd_offset * sizeof(pd_t)) );
							
		pte = (pt_t*)((frm_tab[replace_frame].pte_base * NBPG) + (frm_tab[replace_frame].pte_offset * sizeof(pt_t)));
		
		pt_frame = pde->pd_base - FRAME0;

	//Start process of swapping out

		// Clear pt to swap frame and reverse mapping
		pte->pt_pres = 0;
		pte->pt_base = NULL;
		frm_tab[replace_frame].pte_base = 0;
		frm_tab[replace_frame].pte_offset = 0;

		
		//Find out and write the tlb page clearing instruction

		//decreement valid pte count in the pt_frame
		frm_tab[pt_frame].fr_refcnt -= 1;

		//If refcnt is 0 for the pt_frame -> no ptes in it -> can be freed
		if(frm_tab[pt_frame].fr_refcnt == 0){
			//invalidate pde
			pde->pd_pres = 0;
			pde->pd_base = NULL;
			
			//reset frm_tab entry of that pt
			frm_tab[pt_frame].fr_status = FRM_UNMAPPED;
			frm_tab[pt_frame].fr_pid = -1;
			frm_tab[pt_frame].fr_vpno = -1;
			//frm_tab[pt_frame].fr_type = FR_PAGE;
			frm_tab[pt_frame].fr_dirty = 0;			
			frm_tab[pt_frame].pte_base = 0;
			frm_tab[pt_frame].pte_offset = 0;
		}

		//Write back the replace frame if its dirty	
//		if(frm_tab[replace_frame].fr_dirty == 1){
			//kprintf("HERE!!!!! %d\n", replace_frame);
			bsm_lookup_result = bsm_lookup(pid, vp*NBPG, &store, &pageth);	
			if(bsm_lookup_result == SYSERR){
				//something is wrong
				kill(pid);
			}
			else{
				content_frame = (char *)((FRAME0 + replace_frame)*NBPG);
				write_bs(content_frame, store, pageth);
				
				//Correct the cq mapping
				//frm_tab[frm_tab[replace_frame].prev].next = frm_tab[replace_frame].next;
				//frm_tab[frm_tab[replace_frame].next].prev = frm_tab[replace_frame].prev;

			}
//		}
		//Clear frame before returning
        	frm_tab[replace_frame].fr_status = FRM_UNMAPPED;
        	frm_tab[replace_frame].fr_pid = -1;
        	frm_tab[replace_frame].fr_vpno = -1;
        	frm_tab[replace_frame].fr_type = FR_PAGE;
        	frm_tab[replace_frame].fr_dirty = 0;
        	frm_tab[replace_frame].pte_base = 0;
        	frm_tab[replace_frame].pte_offset = 0;

		// Correct the cq mapping
		if(cq_head == replace_frame){
			cq_head = frm_tab[replace_frame].next;
		}
		if(cq_rear == replace_frame){
			cq_rear = frm_tab[replace_frame].prev;
		}

		frm_tab[frm_tab[replace_frame].prev].next = frm_tab[replace_frame].next;
		frm_tab[frm_tab[replace_frame].next].prev = frm_tab[replace_frame].prev;

		frm_tab[replace_frame].next = -1;
		frm_tab[replace_frame].prev = -1;

		// Correct fifo mapping
		if(fifo_head == replace_frame){
			fifo_head = frm_tab[replace_frame].fifo_next;
		}
		if(fifo_rear == replace_frame){
			fifo_rear = frm_tab[replace_frame].fifo_prev;
		}

		frm_tab[frm_tab[replace_frame].fifo_prev].fifo_next = frm_tab[replace_frame].fifo_next;
		frm_tab[frm_tab[replace_frame].fifo_next].fifo_prev = frm_tab[replace_frame].fifo_prev;

		*avail = replace_frame;
		return replace_frame;
	}
  //return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
	int bs_lookup_result, store, pageth;
	char * content_frame;

	if(frm_tab[i].fr_type == FR_PAGE){
		bs_lookup_result = bsm_lookup(frm_tab[i].fr_pid, frm_tab[i].fr_vpno, &store, &pageth);

                if(bs_lookup_result != SYSERR){
			if(frm_tab[i].fr_dirty == 1){
                		content_frame = (char *)((FRAME0 + i)*NBPG);
                        	write_bs(content_frame, store, pageth);

                        	bsm_tab[store].bs_pids[pageth] = -1;
                        	bsm_tab[store].bs_npages[pageth] = -1;
			}
                }

	}

        frm_tab[i].fr_status = FRM_UNMAPPED;
        frm_tab[i].fr_pid = -1;
        frm_tab[i].fr_vpno = -1;
        frm_tab[i].fr_refcnt = 0;
        frm_tab[i].fr_dirty = 0;


  //kprintf("To be implemented!\n");
  return OK;
}


SYSCALL page_replacement_policy(){

	int i = cq_head;
	int j = fifo_head;
	int frm_id;
	int smallest_age = frm_tab[fifo_head].age;
	int pos = fifo_head;
	pt_t *pte;

	switch(grpolicy()){

	case SC:
		//int i = cq_head;
		//int frm_id;
		//pt_t *pte;
		//use i to iterate through the circular queue tracing next of each element.
		//when I see an element whose pte has pt_acc = 0, then swap
				
		//pte = (pt_t*)((frm_tab[i].pte_base * NBPG) + (frm_tab[i].pte_offset * sizeof(pt_t)));
		//frm_id = circular_queue[i].frame_id; //get the corresponding id of the frame in frm_tab

		frm_id = i;
		pte = (pt_t*)((frm_tab[frm_id].pte_base * NBPG) + (frm_tab[frm_id].pte_offset * sizeof(pt_t))); //pte of the head frame
		
		while(pte->pt_acc == 1){
			pte->pt_acc = 0;

//			i = circular_queue[i].next;
//			frm_id = circular_queue[i].frame_id;

			frm_id = frm_tab[i].next;
			i = frm_id;
			//get pte of next frame
			pte = (pt_t*)((frm_tab[frm_id].pte_base * NBPG) + (frm_tab[frm_id].pte_offset * sizeof(pt_t)));
		}

		


		return frm_id;
		
		//exits out of while loop only when pt_acc = 0

		//i is the position of the swap frame in circular queue
		//frm_id is the id of the swap frame in frm_tab
		//pte is the address of the pte of this resident page/swap frame		

		break;
	case AGING:
		//kprintf("HERE!!!!! AGAINNGNGG\n" );
		frm_id = j;
		volatile int iter = j;
		pte = (pt_t*)((frm_tab[frm_id].pte_base * NBPG) + (frm_tab[frm_id].pte_offset * sizeof(pt_t)));

		while(iter != -1){
			frm_tab[iter].age = frm_tab[iter].age/2;
			iter = frm_tab[iter].fifo_next;
		}

		iter = frm_id;

		while(iter != -1){
			if(pte->pt_acc == 1){
				if(frm_tab[iter].age + 128 > 255){
					frm_tab[iter].age = 255;
				}
				else{
					frm_tab[iter].age += 128;
				}
				pte->pt_acc = 0;
			}
				iter = frm_tab[iter].fifo_next;
				pte = (pt_t*)((frm_tab[iter].pte_base * NBPG) + (frm_tab[iter].pte_offset * sizeof(pt_t)));
		}

		iter = frm_id;
		while(iter != -1){
			if(frm_tab[iter].age < smallest_age){
				smallest_age = frm_tab[iter].age;
				pos = frm_tab[frm_tab[iter].fifo_prev].fifo_next;
			}
			iter = frm_tab[iter].fifo_next;
		}
		//kprintf("Returning POSSSS :%d\n", pos);	
		return pos;
		break;
	default:
		// WILL IT COME HERE?
		kprintf(" DEFAULT POLICY??????");
		break;
	}

}

SYSCALL valid_for_replacement(int frame_i){
	return ((frm_tab[frame_i].fr_type == FR_PAGE) || (frm_tab[frame_i].fr_type == FR_TBL && frm_tab[frame_i].fr_refcnt == 0));
}

SYSCALL frame_lookup_for_vpno(int vpno){
	int i;
	for(i = 0; i < NFRAMES; i++){
		if(frm_tab[i].fr_type == FR_PAGE && frm_tab[i].fr_vpno == vpno){
			return i;
		}	
	}
	return SYSERR;
}

/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
	unsigned long faulted_address = 0;
	int vp = 0, pt_free_frame = -1, page_free_frame = -1;
	unsigned long pd = 0;
	int lookup_result = 0, store = 0, pageth = 0;
	virt_addr_t *fault_addr_struct;
	int pd_i =0,pt_i = 0;
	pd_t *pde;
	pt_t *pte;
	char *content_frame;

	faulted_address = read_cr2();

	//Converting vaddr to vpgno;	
	vp = faulted_address/NBPG;

	//kprintf("\nFaulted Address: %ld\n", faulted_address);

	// Address of the process's page directory
	pd = proctab[currpid].pdbr; //absolute address of pdbr;

	//Put the vaddr in the struct format
	/* This wont work as the struct has allocated only 10:10:12 bits. Doing like this means we are giving 3 ints (96 bits)
	fault_addr_struct.pg_offset = faulted_address & 0x00000fff;
	fault_addr_struct.pt_offset = (faulted_address >> 12) & 0x000003ff;
	fault_addr_struct.pd_offset = (faulted_address >> 22) & 0x000003ff;
	*/

	//fault_addr_struct = (virt_addr_t)(faulted_address);
	fault_addr_struct = (virt_addr_t*)(&faulted_address);
	lookup_result = bsm_lookup(currpid, faulted_address, &store, &pageth);	

	if(lookup_result == SYSERR){
		//The faulted address is not in Backing store too. So error.
		kprintf("The process is trying to access illegal address. Killing it!\n");
		//kill using currpid?
		//kill(currpid);
		kill(currpid);
	}

	else{
		//Step 1: Check if pde.present = 1 -> The PT(page of PT) for this is present in memory	
		//a) First go to that pde in PD
		//pde = pd;

		pde = (pd_t*)(proctab[currpid].pdbr + (fault_addr_struct->pd_offset * sizeof(pd_t)) ); 

		/*
		while(pd_i != fault_addr_struct->pd_offset){
			pd_i++;
			pde++;		
		}
		*/
		// Go to the specific pde that is associated with this virtaddr;
		//pde += fault_addr_struct->pd_offset;

		// Setup page table if its not there
		if(pde->pd_pres == 0){
			// Create and set frame for the page table
			get_frm(&pt_free_frame);
			frm_tab[pt_free_frame].fr_status = FRM_MAPPED;
			frm_tab[pt_free_frame].fr_pid = currpid;
			frm_tab[pt_free_frame].fr_vpno = -1; // Not applicable
			frm_tab[pt_free_frame].fr_refcnt = 1;
			frm_tab[pt_free_frame].fr_type = FR_TBL;
			frm_tab[pt_free_frame].fr_dirty = 0;

			//Initialize ptes in that Page Table
			pte = (pt_t*)((FRAME0 + pt_free_frame) * NBPG);
			for(pt_i = 0; pt_i < NPTES; pt_i++){
				pte->pt_pres = 0;
				pte->pt_write = 1;
				pte->pt_user = 0;
				pte->pt_pwt = 0;
				pte->pt_pcd = 0;
				pte->pt_acc = 0;
				pte->pt_dirty = 0;
				pte->pt_mbz = 0;
				pte->pt_global = 0;
				pte->pt_avail = 0;
				pte->pt_base = -1;//always check with pt_pres before accessing pt_base	
				++pte;
			}

			pde->pd_pres = 1;
			pde->pd_base = FRAME0 + pt_free_frame;
		
		}

		//pte = pde->pd_base * NBPG; //Just intermediate allocaiton.
		//pte = (NFRAME + pt_free_frame) * NBPG;

//		changing pte = (pt_t*)(((FRAME0 + pt_free_frame) * NBPG) + (fault_addr_struct->pt_offset * sizeof(pt_t)));

		pte = (pt_t*)(((pde->pd_base) * NBPG) + (fault_addr_struct->pt_offset * sizeof(pt_t)));
		//pte += fault_addr_struct->pt_offset;
		
		lookup_result = bsm_lookup(currpid, faulted_address, &store, &pageth); //will be OK only here.
		frm_tab[pde->pd_base - FRAME0].fr_refcnt++;


		if(pte->pt_pres != 1){
			get_frm(&page_free_frame);


			//setup content frame
			frm_tab[page_free_frame].fr_status = FRM_MAPPED;
			frm_tab[page_free_frame].fr_pid = currpid;
			frm_tab[page_free_frame].fr_vpno = vp;
			frm_tab[page_free_frame].fr_refcnt = 1;
			frm_tab[page_free_frame].fr_type = FR_PAGE;
			frm_tab[page_free_frame].fr_dirty = 0;
		
			content_frame = (char *)((FRAME0 + page_free_frame)*NBPG); //address
			read_bs(content_frame, store, pageth);

			pte->pt_base = FRAME0 + page_free_frame;
			pte->pt_pres = 1;

			//Required mapping for the cirucular queue
			
			//first time into cq
			if(cq_head == -1 && cq_rear == -1){
				cq_head = page_free_frame;
				cq_rear = page_free_frame;
				frm_tab[page_free_frame].next = page_free_frame;
				frm_tab[page_free_frame].prev = page_free_frame;
			}
			else{
				frm_tab[cq_rear].next = page_free_frame;
				frm_tab[page_free_frame].prev = cq_rear;

				cq_rear = page_free_frame; //Put in frame.c

				frm_tab[cq_rear].next = cq_head;
				frm_tab[cq_head].prev = cq_rear;
			}
			// Reverse mapping from the frame to the corresponding PTE
			frm_tab[page_free_frame].pte_base = pde->pd_base;
			frm_tab[page_free_frame].pte_offset = fault_addr_struct->pt_offset;

			// Required mapping for fifo queue
			
			if(fifo_head == -1 && fifo_rear == -1){
				fifo_head = page_free_frame;
				fifo_rear = page_free_frame;

				frm_tab[page_free_frame].fifo_next = -1;
				frm_tab[page_free_frame].fifo_prev = -1;
			}

			else{
				frm_tab[fifo_rear].fifo_next = page_free_frame;
				frm_tab[page_free_frame].fifo_prev = fifo_rear;

				fifo_rear = page_free_frame;

				frm_tab[fifo_rear].fifo_next = -1;
			}
		}		
/*
			// Create and set frame for the page
			get_frm(&page_free_frame);
			frm_tab[page_free_frame].fr_status = FRM_MAPPED;
			frm_tab[page_free_frame].fr_pid = currpid;
			frm_tab[page_free_frame].fr_vpno = vp
			frm_tab[page_free_frame].fr_refcnt = 1;
			frm_tab[page_free_frame].fr_type = FR_PAGE;
			frm_tab[page_free_frame].fr_dirty = 0;
			
			pte->pt_base = FRAME0 + page_free_frame;
			pte->pt_pres = 1;
*/
			/* So, we found a free frame for a new page table, initialized the frame, initialized the ptes in the page table,
 			mapped the corresponding pde to the new page table and set present = 1.
			Then, we found a free frame for the new page, inititalized the frame, mapped the corresponding pte to the new page
			and set present  = 1;
			Now we have to retry the instruction that resulted in page fault. */
						
			

	}
	write_cr3(proctab[currpid].pdbr);
	//How to retry the instruction ???????
   	//kprintf("To be implemented!\n");
	return OK;
}



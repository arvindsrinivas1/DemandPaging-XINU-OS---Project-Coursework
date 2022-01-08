/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>
/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
	volatile int i, j, store, pageth, bs_lookup_result;
	char *content_frame;



	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);


        for(i = 0; i < NFRAMES; i++){
                 if(frm_tab[i].fr_pid == pid){
                         if(frm_tab[i].fr_type == FR_PAGE){
                                 bs_lookup_result = bsm_lookup(pid, frm_tab[i].fr_vpno * NBPG, &store, &pageth);

                                 if(bs_lookup_result != SYSERR){
                                         content_frame = (char *)((FRAME0 + i)*NBPG);
                                         write_bs(content_frame, store, pageth);

                                         bsm_tab[store].bs_pids[pageth] = -1;
                                         bsm_tab[store].bs_npages[pageth] = -1;
                                 }
                         }

                        
                         frm_tab[i].fr_status = FRM_UNMAPPED;
                         frm_tab[i].fr_pid = -1;
                         frm_tab[i].fr_vpno = -1;
                         frm_tab[i].fr_refcnt = 0;
                         frm_tab[i].fr_dirty = 0;
                 }
         }

         if(proctab[i].store != -1){
                 release_bs(proctab[i].store);
         }

         for(i = 0; i < 8 ; i++){
                 for(j = 0; j < MAX_BS_PAGES; j++){
                         if(bsm_tab[i].bs_pids[j] == pid){
                                 bsm_tab[i].bs_pids[j] = -1;
                                 bsm_tab[i].bs_npages[j] = -1;
                         }
                 }
         }


	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	//
	/*
	for(i = 0; i < NFRAMES; i++){
		if(frm_tab[i].fr_pid == pid){
			if(frm_tab[i].fr_type == FR_PAGE){
				bs_lookup_result = bsm_lookup(pid, frm_tab[i].fr_vpno, &store, &pageth);

				if(bs_lookup_result != SYSERR){
					content_frame = (char *)((FRAME0 + i)*NBPG);
					write_bs(content_frame, store, pageth);
				
                        		bsm_tab[store].bs_pids[pageth] = -1;
                        		bsm_tab[store].bs_npages[pageth] = -1;
				}
			}

			// Free the frame for pages, pgtbl and pgdir
                        frm_tab[i].fr_status = FRM_UNMAPPED;
                        frm_tab[i].fr_pid = -1;
                        frm_tab[i].fr_vpno = -1;
                        frm_tab[i].fr_refcnt = 0;
                        frm_tab[i].fr_dirty = 0;
		}
	}

	if(proctab[i].store != -1){ //If the process is created by vcreate and has a vheap backing store
		release_bs(proctab[i].store);
	}

	for(i = 0; i < 8 ; i++){
		for(j = 0; j < MAX_BS_PAGES; j++){
			if(bsm_tab[i].bs_pids[j] == pid){
				bsm_tab[i].bs_pids[j] = -1;
				bsm_tab[i].bs_npages[j] = -1;
			}
		}
	}
	*/

	restore(ps);
	return(OK);
}

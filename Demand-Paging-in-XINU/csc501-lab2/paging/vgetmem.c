/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 * vgetmem  --  allocate virtual heap storage, returning lowest WORD address
 *------------------------------------------------------------------------
 */
WORD	*vgetmem(nbytes)
	unsigned nbytes;
{

/*
 	TEST WITH THIS METHOD LATER
	int i = 0, store = -1, pageth = -1, filled_size = 0;
	//struct mblock *bs_ptr = proctab[currpid].vmemlist->mnext;
	unsigned long backing_store_base = BACKING_STORE_BASE;
	unsigned long backing_store_unit_size = BACKING_STORE_UNIT_SIZE;	
	int npages = bytes_to_pages(nbytes);
	store = ((int *)(proctab[currpid].vmemlist.mnext) - BACKING_STORE_BASE) / (BACKING_STORE_UNIT_SIZE);
	//Assuming no holes, again.	
	

	while( bsm_tab[store].bs_pids[i] != -1){
		++i;
	}

	filled_size = i;	
	
	if(filled_size == 256 || ((i + (npages - 1)) > 255)){
	//BS already full or newely requested bytes exceeds the limits of the backing store
		return SYSERR;
	}
*/
	/* Not needed as we already initialized during vcreate
	for(i = 0; i < npages; i++){
		bsm_tab[store].bs_pids[filled_size + i] = pid;
		bsm_tab[store].bs_npages[filled_size + i] =  
	}
	*/
	STATWORD ps;
	struct mblock *p, *q, *leftover;
	disable(ps);

        if (nbytes==0 || proctab[currpid].vmemlist.mnext== (struct mblock *) NULL) {
                restore(ps);
                return( (WORD *)SYSERR);
        }

	nbytes = (unsigned int) roundmb(nbytes);

	for(q = &proctab[currpid].vmemlist, p = proctab[currpid].vmemlist.mnext; p != (struct mblock *) NULL; q=p,p=p->mnext){
                if ( q->mlen == nbytes) {
                        q->mnext = p->mnext;
                        restore(ps);
                        return( (WORD *)p );
                }
		else if ( q->mlen > nbytes ) {
                        leftover = (struct mblock *)( (unsigned)p + nbytes );
                        q->mnext = leftover;
                        leftover->mnext = p->mnext;
                        leftover->mlen = q->mlen - nbytes;
                        restore(ps);
                        return( (WORD *)p );
                }
		restore(ps);
		return((WORD *)SYSERR);

	}

}


SYSCALL bytes_to_pages(unsigned nbytes){
	int npages = nbytes/NBPG;
	float f_npages = nbytes/NBPG;
	
	if(f_npages > npages){
		npages += 1;
	}
	return npages;	
}

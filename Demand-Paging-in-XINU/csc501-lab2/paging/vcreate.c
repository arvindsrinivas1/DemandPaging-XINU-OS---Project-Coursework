/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	int pid,i, map_result;
	bsd_t bs_id;
	struct mblock *vmemlist;

//	kprintf("To be implemented!\n");

	pid = create(procaddr, ssize, priority, name, nargs, args); //disables + renables interrupt inside that so we don't have to do it here, I guess.
	get_bsm(&bs_id);	
	
	if(bs_id == SYSERR){
		return SYSERR;
	}
	//else
	// We have a free Backing Store. Now we have to change the entry in bsm_tab

	get_bs(bs_id, hsize); //Oblivious to PID. Sets Vheap to 0 so we have to change it now.
	bsm_tab[bs_id].bs_vheap_check = IS_VHEAP;
	//set values
	/*
	for(i = 0; i < hsize; i++){
		bsm_tab[bs_id] = IS_VHEAP;
		set_bsm_tab_pids(bs_id, pid);
		set_bsm_tab_npages(bs_id, 4096);
	}
	*/
	map_result = bsm_map(pid, 4096, bs_id, hsize);
	if(map_result == SYSERR){
		//it will return SYSERR only if overflow but for new bs, it can never happen
		//So, ideally it should never come inside this.
		return SYSERR;
	}
	//changes in proctab
	//proctab[pid].pdbr = ;
	proctab[pid].store = bs_id;
	proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;
	//proctab[pid].vmemlist->mnext = (struct mblock*)(get_first_addr_bs(bs_id));
	//proctab[pid].vmemlist->mnext =  proctab[pid].vhpno * NBPG;
	//proctab[pid].vmemlist->mlen = bsm_tab[bs_id].bs_size;

	proctab[pid].vmemlist.mnext = (struct mblock*)(proctab[pid].vhpno * NBPG);
	proctab[pid].vmemlist.mlen =  bsm_tab[bs_id].bs_size * NBPG;
	

	//proctab[pid].vmemlist->mnext = 4096 * NBPG;

	//proctab[pid].vmemlist.mlen = bsm_tab[bs_id].bs_size * NBPG;

	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}

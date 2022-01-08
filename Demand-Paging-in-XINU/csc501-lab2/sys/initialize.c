/* initialize.c - nulluser, sizmem, sysinit */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <sleep.h>
#include <mem.h>
#include <tty.h>
#include <q.h>
#include <io.h>
#include <paging.h>

/*#define DETAIL */
#define HOLESIZE	(600)	
#define	HOLESTART	(640 * 1024)
#define	HOLEEND		((1024 + HOLESIZE) * 1024)  
/* Extra 600 for bootp loading, and monitor */

extern	int	main();	/* address of user's main prog	*/

extern	int	start();

LOCAL		sysinit();

/* Declarations of major kernel variables */
struct	pentry	proctab[NPROC]; /* process table			*/
int	nextproc;		/* next process slot to use in create	*/
struct	sentry	semaph[NSEM];	/* semaphore table			*/
int	nextsem;		/* next sempahore slot to use in screate*/
struct	qent	q[NQENT];	/* q table (see queue.c)		*/
int	nextqueue;		/* next slot in q structure to use	*/
char	*maxaddr;		/* max memory address (set by sizmem)	*/
struct	mblock	memlist;	/* list of free memory blocks		*/
bs_map_t bsm_tab[8];      	/* Backing store map data structure	*/
fr_map_t frm_tab[NFRAMES];	/* List of frames */




int cq_head;

int cq_rear;


int fifo_head = -1;
int fifo_rear = -1;


#ifdef	Ntty
struct  tty     tty[Ntty];	/* SLU buffers and mode control		*/
#endif

/* active system status */
int	numproc;		/* number of live user processes	*/
int	currpid;		/* id of currently running process	*/
int	reboot = 0;		/* non-zero after first boot		*/

int	rdyhead,rdytail;	/* head/tail of ready list (q indicies)	*/
char 	vers[80];
int	console_dev;		/* the console device			*/

/*  added for the demand paging */
int page_replace_policy = SC;
int policy_debugger = 0;
/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED, and      ***/
/***   must eventually be enabled explicitly.  This routine turns     ***/
/***   itself into the null process after initialization.  Because    ***/
/***   the null process must always remain ready to run, it cannot    ***/
/***   execute code that might cause it to be suspended, wait for a   ***/
/***   semaphore, or put to sleep, or exit.  In particular, it must   ***/
/***   not do I/O unless it uses kprintf for polled output.           ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  nulluser  -- initialize system and become the null process (id==0)
 *------------------------------------------------------------------------
 */
nulluser()				/* babysit CPU when no one is home */
{
        int userpid;

	console_dev = SERIAL0;		/* set console to COM0 */

	initevec();

	kprintf("system running up!\n");
	sysinit();

	enable();		/* enable interrupts */

	sprintf(vers, "PC Xinu %s", VERSION);
	kprintf("\n\n%s\n", vers);
	if (reboot++ < 1)
		kprintf("\n");
	else
		kprintf("   (reboot %d)\n", reboot);


	kprintf("%d bytes real mem\n",
		(unsigned long) maxaddr+1);
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) 0);
	kprintf(" to %d\n", (unsigned long) (maxaddr) );
#endif	

	kprintf("%d bytes Xinu code\n",
		(unsigned long) ((unsigned long) &end - (unsigned long) start));
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) start);
	kprintf(" to %d\n", (unsigned long) &end );
#endif

#ifdef DETAIL	
	kprintf("%d bytes user stack/heap space\n",
		(unsigned long) ((unsigned long) maxaddr - (unsigned long) &end));
	kprintf("    %d", (unsigned long) &end);
	kprintf(" to %d\n", (unsigned long) maxaddr);
#endif	
	
	kprintf("clock %sabled\n", clkruns == 1?"en":"dis");


	/* create a process to execute the user's main program */
	userpid = create(main,INITSTK,INITPRIO,INITNAME,INITARGS);
	resume(userpid);

	while (TRUE)
		/* empty */;
}

/*------------------------------------------------------------------------
 *  sysinit  --  initialize all Xinu data structeres and devices
 *------------------------------------------------------------------------
 */
LOCAL
sysinit()
{
	static	long	currsp;
	int	i,j;
	struct	pentry	*pptr;
	struct	sentry	*sptr;
	struct	mblock	*mptr;
	pt_t *pte;
	pd_t *pde;
	int global_pt_i = 0, null_pd = 0;
	int page_replace_policy = SC;

	SYSCALL pfintr();

	

	numproc = 0;			/* initialize system variables */
	nextproc = NPROC-1;
	nextsem = NSEM-1;
	nextqueue = NPROC;		/* q[0..NPROC-1] are processes */
	
	//NOTE: Install Page Fault handler
	set_evec(14, pfintr);


	// Initializing global Ciruclar Queue variables

	cq_head = -1;
	cq_rear = -1;

	/* initialize free memory list */
	/* PC version has to pre-allocate 640K-1024K "hole" */
	if (maxaddr+1 > HOLESTART) {
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = (struct mblock *)HOLEEND;
		mptr->mlen = (int) truncew(((unsigned) HOLESTART -
	     		 (unsigned)&end));
        mptr->mlen -= 4;

		mptr = (struct mblock *) HOLEEND;
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - HOLEEND -
	      		NULLSTK);
/*
		mptr->mlen = (int) truncew((unsigned)maxaddr - (4096 - 1024 ) *  4096 - HOLEEND - NULLSTK);
*/
	} else {
		/* initialize free memory list */
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - (int)&end -
			NULLSTK);
	}
	

	for (i=0 ; i<NPROC ; i++)	/* initialize process table */
		proctab[i].pstate = PRFREE;


#ifdef	MEMMARK
	_mkinit();			/* initialize memory marking */
#endif

#ifdef	RTCLOCK
	clkinit();			/* initialize r.t.clock	*/
#endif

	mon_init();     /* init monitor */

#ifdef NDEVS
	for (i=0 ; i<NDEVS ; i++ ) {	    
	    init_dev(i);
	}
#endif

	pptr = &proctab[NULLPROC];	/* initialize null process entry */
	pptr->pstate = PRCURR;
	for (j=0; j<7; j++)
		pptr->pname[j] = "prnull"[j];
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK;
	pptr->pbase = (WORD) maxaddr - 3;
/*
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK - (4096 - 1024 )*4096;
	pptr->pbase = (WORD) maxaddr - 3 - (4096-1024)*4096;
*/
	pptr->pesp = pptr->pbase-4;	/* for stkchk; rewritten before used */
	*( (int *)pptr->pbase ) = MAGIC;
	pptr->paddr = (WORD) nulluser;
	pptr->pargs = 0;
	pptr->pprio = 0;
	currpid = NULLPROC;

	for (i=0 ; i<NSEM ; i++) {	/* initialize semaphores */
		(sptr = &semaph[i])->sstate = SFREE;
		sptr->sqtail = 1 + (sptr->sqhead = newqueue());
	}

	rdytail = 1 + (rdyhead=newqueue());/* initialize ready list */

	init_bsm();
	init_frm();

	//NOTE: GLOBAL PAGE TABLE CREATION
	init_frames_for_global_pts();	
	//pte = (pt_t*)((NFRAME0 + global_pt_i)* NBPG);
	//setting the actual PTE values in the physical memory
	for(global_pt_i = 0 ; global_pt_i < 4 ; global_pt_i++){
		//set pte for global page tables
		pte = (pt_t*)((FRAME0 + global_pt_i)* NBPG);
		for(i = 0; i< NPTES; i++){
			pte->pt_pres = 1; //always present
			pte->pt_write = 1; //wont write after this but still setting 1
			pte->pt_user = 0;
			pte->pt_pwt = 0;
			pte->pt_pcd = 0;
			pte->pt_acc = 0; // just setting now. not accessing. So 0.
			pte->pt_dirty = 0;
			pte->pt_mbz = 0;
			pte->pt_global = 0;
			pte->pt_avail = 0;
			//pte->pt_base = (FRAME0 * (global_pt_i + 1));//+ i note needed 
			pte->pt_base = (FRAME0 * global_pt_i) + i; // 0-1023 / 1024 - 2047 / 2048 - 3071/ 3072-4095
			pte++;
		}
	}

	//check here if pte here is at 4096 * NBPG

	//NOTE: PD CREATION FOR NULLPROC
	null_pd = init_frame_for_pd_tab(); // should always be 4(0-3 are Global PTs and 4 is PD of NULLPROC)
	//null_pd is realtive index. Not absolute frame.



	//SET VHEAP RELATED THINGS IN PROCTAB for NULLPROC
	pptr->pdbr = (FRAME0 + null_pd) * NBPG; // set the pdbr to absolute address.
	pptr->store = -1;
	pptr->vhpno = -1;
	pptr->vhpnpages = 0;
	//pptr->vmemlist = NULL;

	pde = (pd_t*)((FRAME0 + null_pd) * NBPG);
	//setting the actual pde values in the physcial memory
	for(i = 0; i < 4; i++){
		pde->pd_pres = 1; // Only when corresponding PT is in memory
		pde->pd_write = 1; // ""
		pde->pd_user = 0;
		pde->pd_pwt = 0;
		pde->pd_pcd = 0;
		pde->pd_acc = 0;
		pde->pd_mbz = 0;
		pde->pd_fmb = 0;
		pde->pd_global = 0;
		pde->pd_avail = 0;
//		if(i<4)
		pde->pd_base = FRAME0 + i;
//		else
//			pde->pd_base = -1;
		pde++;
	}

	write_cr3(pptr->pdbr);
	enable_paging();
	
	return(OK);
}

stop(s)
char	*s;
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* empty */;
}

delay(n)
int	n;
{
	DELAY(n);
}


#define	NBPG	4096

/*------------------------------------------------------------------------
 * sizmem - return memory size (in pages)
 *------------------------------------------------------------------------
 */
long sizmem()
{
	unsigned char	*ptr, *start, stmp, tmp;
	int		npages;

	/* at least now its hacked to return
	   the right value for the Xinu lab backends (16 MB) */

	return 4096; 

	start = ptr = 0;
	npages = 0;
	stmp = *start;
	while (1) {
		tmp = *ptr;
		*ptr = 0xA5;
		if (*ptr != 0xA5)
			break;
		*ptr = tmp;
		++npages;
		ptr += NBPG;
		if ((int)ptr == HOLESTART) {	/* skip I/O pages */
			npages += (1024-640)/4;
			ptr = (unsigned char *)HOLEEND;
		}
	}
	return npages;
}

SYSCALL init_frame_for_pd_tab(){
	//search for a free frame
	//FRAME0 + 4 for the PD of null proc
	int i, frame_pos = 0;
	for(i = 0; i<NFRAMES; i++){
		if(frm_tab[i].fr_status == FRM_UNMAPPED){
			frame_pos = i;			
			frm_tab[i].fr_status = FRM_MAPPED;
			frm_tab[i].fr_pid = NULLPROC;
			//doesnt matter - Page Direct.ifr_vpno = ;
			frm_tab[i].fr_refcnt = 1;
			frm_tab[i].fr_type = FR_DIR;
			frm_tab[i].fr_dirty = 0;
			break;
		}
	}
	if(frame_pos == 0){
		// Page Replacement Policy
	}
	if(frame_pos != 0)
		return frame_pos;
	return SYSERR; // NEVER HAPPENS ? Because we will get the page somehow.
}

SYSCALL	init_frames_for_global_pts(){
	//search for free frame
	// Basically sets FRAME0 to FRAME0 + 3 to the global pts.
	int i, j;	

	for(j = 0; j < 4 ; j++){
		for(i=0; i<NFRAMES; i++){
			if(frm_tab[i].fr_status == FRM_UNMAPPED){
				//frame_pos = i;
                        	frm_tab[i].fr_status = FRM_MAPPED;
                        	frm_tab[i].fr_pid = NULLPROC; //not accurate but meh.
                        	frm_tab[i].fr_vpno = -1;  //??;
                        	frm_tab[i].fr_type = FR_TBL;
				frm_tab[i].fr_dirty = 0;
				frm_tab[i].fr_refcnt = 1;
				break;
			}
			else{
				//Page replacement. Same as above
				// Wont need here.
			}	
		}
	}

	return OK;
}

SYSCALL define_global_pts_frames(){
	int i;
	for(i = 0; i<4; i++){
        	frm_tab[i].fr_status = FRM_MAPPED;
        	frm_tab[i].fr_pid = NULLPROC; //not accurate but meh.
        	frm_tab[i].fr_vpno = -1;  //??;
        	frm_tab[i].fr_type = FR_TBL;
        	frm_tab[i].fr_dirty = 0;
        	frm_tab[i].fr_refcnt = 1;
	}
	return OK;

}

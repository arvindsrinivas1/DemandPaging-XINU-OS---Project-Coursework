/* paging.h */

typedef unsigned int	 bsd_t;

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct{
  int bs_status;			/* MAPPED or UNMAPPED		*/
  int bs_vheap_check;			/* 1 if BS is a vheap, else 0   */
  int bs_pids[256];			/* process ids in each page of this slot.  */
//  int bs_vpno;				/* starting virtual page number */
  int bs_npages[256];			/* vpg in each page of the slot. couple with bs_pidss*/
  int bs_sem;				/* semaphore mechanism ?	*/
  int bs_size;
} bs_map_t;

typedef struct{
  int fr_status;			/* MAPPED or UNMAPPED		*/
  int fr_pid;				/* process id using this frame  */
  int fr_vpno;				/* corresponding virtual page no*/
  int fr_refcnt;			/* reference count		*/
  int fr_type;				/* FR_DIR, FR_TBL, FR_PAGE	*/
  int fr_dirty;				/* Page has been modified since it was brought into memory => 1 (OSTEP) */
  unsigned int pte_base;		/* Frame of pte for this frame */ 
  unsigned int pte_offset;		/* Offset of pte in the above frame */

  /* Modification for cq */
  int next;
  int prev;

  /* Modification for fifoq */
  int fifo_next;
  int fifo_prev;
  int age;
}fr_map_t;

extern bs_map_t bsm_tab[];
extern fr_map_t frm_tab[];



/* DS for Page Replacement Policies */

struct cqentry{
  int frame_id;
  int next; 
};



//extern struct cqentry circular_queue[];
//extern int cq_curr_pos; 
extern int cq_head;
extern int cq_rear;
//extern int cq_insert_ptr;
//extern int cq_size;
//extern int pr_policy;

extern int fifo_head;
extern int fifo_rear;


/* Prototypes for required API calls */
SYSCALL xmmap(int, bsd_t, int);
SYSCALL xunmap(int);

/* given calls for dealing with backing store */

int get_bs(bsd_t, unsigned int);
SYSCALL release_bs(bsd_t);
SYSCALL read_bs(char *, bsd_t, int);
SYSCALL write_bs(char *, bsd_t, int);



/*Custom calls for dealing with backing store */
void reset_bsm_entry(int i);
SYSCALL check_bs_overflow(bsd_t source, int npages);
SYSCALL init_bsm_tab_npages(int entry);
SYSCALL init_bsm_tab_pids(int entry);
SYSCALL set_bsm_tab_pids(int entry, int pid); //Only for vcreate();
SYSCALL set_bsm_tab_npages(int entry, int vpgno); // Only for vcreate();


#define NBPG		4096	/* number of bytes per page	*/
#define FRAME0		1024	/* zero-th frame		*/
#define NFRAMES 	1024	/* number of frames		*/

#define NPDES		1024	/* No of PDEs in a PD		*/
#define NPTES		1024	/* No of PTEs in a PT page	*/

#define BSM_UNMAPPED	0
#define BSM_MAPPED	1

#define FRM_UNMAPPED	0
#define FRM_MAPPED	1

#define IS_VHEAP	1
#define NOT_VHEAP	0

#define FR_PAGE		0
#define FR_TBL		1
#define FR_DIR		2

#define SC 3
#define AGING 4

#define MAX_BS_PAGES 	256
#define BS_STARTING_PAGE	2048
#define BACKING_STORE_BASE	0x00800000
#define BACKING_STORE_UNIT_SIZE 0x00100000

/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
 // kprintf("xmmap - to be implemented!\n");
  int mapping_result;

  mapping_result = bsm_map(currpid, virtpage, source, npages);

  return mapping_result;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
//  kprintf("To be implemented!");
  bsm_unmap(currpid, virtpage, 0);
  return SYSERR;
}

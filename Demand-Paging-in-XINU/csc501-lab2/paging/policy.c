/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */

extern int page_replace_policy;
extern int policy_debugger;


SYSCALL srpolicy(int policy)
{
  /* sanity check ! */
  if(policy != AGING && policy != SC){
	// Do nothing. Keep the existing policy

	//kprintf("WHAT!!?????  ---- %d\n ", policy);
  }
  else{
	//kprintf("UPDATING POLICY");
	policy_debugger = 1;
	page_replace_policy = policy;
	//kprintf("NEW POLiCY 1: %d, 2: %d", policy, page_replace_policy);
  }
  kprintf("To be implemented!\n");

  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

SYSCALL get_debugger(){
	return policy_debugger;
}


#include "contiki.h"
#include "net/rpl/rpl-private.h"
#include "net/uip.h"
#include "net/uip-nd6.h"
#include "net/nbr-table.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/ctimer.h"
#include "bcp.h"

#include <limits.h>
#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/uip-debug.h"






static struct ctimer bcp_periodic_timer;
static struct ctimer bcp_beacon_timer;

//#if UIP_CONF_IPV6
/*---------------------------------------------------------------------------*/
//extern rpl_of_t RPL_OF;
//static rpl_of_t * const objective_functions[] = {&RPL_OF};

/*---------------------------------------------------------------------------*/
/* RPL definitions. */

//#ifndef RPL_CONF_GROUNDED
//#define RPL_GROUNDED                    0
//#else
//#define RPL_GROUNDED                    RPL_CONF_GROUNDED
//#endif /* !RPL_CONF_GROUNDED */

/*---------------------------------------------------------------------------*/
/* Per-parent RPL information */
NBR_TABLE(bcp_parent_t, bcp_parents);    //#define NBR_TABLE(type, name)   //making a neighbor table




//declarations of the neighbors







//definitions of the parents
uip_ipaddr_t *
bcp_get_parent_ipaddr(bcp_parent_t *p)
{
  rimeaddr_t *lladdr = nbr_table_get_lladdr(bcp_parents, p);
  return uip_ds6_nbr_ipaddr_from_lladdr((uip_lladdr_t *)lladdr);
}




void
bcp_init(void)
{
  printf("bcp.c: bcp is being initialized\n");
  uip_ipaddr_t rplmaddr;
  PRINTF("RPL started\n");
  //default_instance = NULL;

  //rpl_dag_init();
  bcp_nbr_init();
  bcp_reset_periodic_timer();

  /* add rpl multicast address */
  uip_create_linklocal_rplnodes_mcast(&rplmaddr);
  uip_ds6_maddr_add(&rplmaddr);


}


void
bcp_reset_periodic_timer(void)
{
 
  ctimer_set(&bcp_periodic_timer, CLOCK_SECOND, bcp_handle_periodic_timer, NULL);
}


void
bcp_handle_periodic_timer(void *ptr)
{
  
  ctimer_reset(&bcp_periodic_timer);
}


void
bcp_nbr_init(void)
{
  printf("bcp.c: We are initializing the bcp neighbor table\n");
  nbr_table_register(bcp_parents, (nbr_table_callback *)bcp_remove_parent);
}


void
bcp_remove_parent(bcp_parent_t *parent)
{
  printf("bcp.c: we are removing a parent from here\n");
  PRINTF("RPL: Removing parent ");
  PRINT6ADDR(bcp_get_parent_ipaddr(parent));
  PRINTF("\n");

  //rpl_nullify_parent(parent);

  nbr_table_remove(bcp_parents, parent);
}


bcp_parent_t *
bcp_add_parent( rpl_dio_t *dio, uip_ipaddr_t *addr)
{
  printf("bcp.c: we are adding a parent over here\n");
  rpl_parent_t *p = NULL;
  /* Is the parent known by ds6? Drop this request if not.
   * Typically, the parent is added upon receiving a DIO. */
  /*uip_lladdr_t *lladdr = uip_ds6_nbr_lladdr_from_ipaddr(addr);

  PRINTF("RPL: rpl_add_parent lladdr %p\n", lladdr);
  if(lladdr != NULL) {

    printf("rpl-dag: This is the information we added in DIO %d\n", dio->queue_size);
    //* Add parent in rpl_parents 
    p = nbr_table_add_lladdr(rpl_parents, (rimeaddr_t *)lladdr);
    p->dag = dag;
    p->rank = dio->rank;
    p->dtsn = dio->dtsn;
    p->link_metric = RPL_INIT_LINK_METRIC * RPL_DAG_MC_ETX_DIVISOR;
#if RPL_DAG_MC != RPL_DAG_MC_NONE
    memcpy(&p->mc, &dio->mc, sizeof(p->mc));
#endif // RPL_DAG_MC != RPL_DAG_MC_NONE 
  }*/

  return p;
}




void
bcp_process_beacon(uip_ipaddr_t *from, rpl_dio_t *dio)
{
 

        printf("bcp process beacon\n");
		    bcp_reset_beacon_timer();
      	
        /*if(dio->prefix_info.length != 0) {
          if(dio->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS) {
            PRINTF("RPL : Prefix announced in DIO\n");
            rpl_set_prefix(dag, &dio->prefix_info.prefix, dio->prefix_info.length);   //prefix is set only the root
          }
        }*/
	  
}


/*---------------------------------------------------------------------------*/
void
handle_bcp_timer()
{
  
      //PRINTF("RPL: Postponing DIO transmission since link local address is not ok\n");
      //ctimer_set(&instance->dio_timer, CLOCK_SECOND, &handle_dio_timer, instance);
      printf("we are in handle bcp timer\n");
      beacon_output(NULL);
  
}


void
bcp_reset_beacon_timer()
{
     printf("resetting the timer\n");
     ctimer_set(&bcp_beacon_timer, CLOCK_SECOND, &handle_bcp_timer, NULL);   //just do this .. maybe think of using reset in its place
     
}

/* 

* This code has been developed as EE652 final project at Viterbi School of Engineering.
* Parts of the code have been adapted from the existing RPL implementation available on Contiki 2.7
* This code shows the basic funcationality  of BCP on IPv6 stack of Contiki

* Authors:
* Chhavi Kapoor ckapoor@usc.edu
* Mrunal Muni muni@usc.edu

*/



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


extern unsigned short node_id;
static struct ctimer bcp_periodic_timer;
static struct ctimer bcp_beacon_timer;

int etx_val = 1;
int link_rate = 1;



/*---------------------------------------------------------------------------*/

NBR_TABLE(bcp_nbr_t, bcp_nbrs);   





/*****************************************************/
/*           Find best next hop from neighbor list     */
/*****************************************************/

bcp_nbr_t *
bcp_find_next_hop()
{
  /*traverse the list of neighbor and find the one with lowest queue size*/
   bcp_nbr_t *nbr = NULL, *temp_nbr= NULL;
   int temp_weight = 0;
   int temp_queue_diff = 0;
   uip_ipaddr_t *ip1 = NULL;
   int etx;
   int q_diff;
   nbr = nbr_table_head(bcp_nbrs);

  while(nbr != NULL) {
      ip1 = bcp_get_nbr_ipaddr(nbr);
      temp_queue_diff = (get_list_length()- nbr->queue_size);
      //printf("BCP: Node ID of neighbor is %d the queue difference is %d and the etx value is %d\n", ip1->u8[15], temp_queue_diff, etx_val);
    if(temp_weight < ((get_list_length()- nbr->queue_size ) - etx_val)*link_rate) {
        temp_weight = (get_list_length()- nbr->queue_size - etx_val)*link_rate;
        temp_nbr =  nbr;
        etx = etx_val;
        q_diff = temp_queue_diff;
      }
      nbr = nbr_table_next(bcp_nbrs, nbr);
    
  }

  if((temp_weight) > 0){

    uip_ipaddr_t *ip = NULL;
  
    ip = bcp_get_nbr_ipaddr(temp_nbr);
     
    return temp_nbr;
  }
  else{
    /*No neighbor with positive weight*/
    return NULL;
  }

}



/*****************************************************/
/*           Find nbr using its IP address           */
/*****************************************************/

bcp_nbr_t *
bcp_find_nbr(uip_ipaddr_t *addr)
{
  uip_ds6_nbr_t *ds6_nbr = uip_ds6_nbr_lookup(addr);
  uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(ds6_nbr);
  return nbr_table_get_from_lladdr(bcp_nbrs, (rimeaddr_t *)lladdr);
}



/*****************************************************/
/*      Find IP address of a given neighbor          */
/*****************************************************/
uip_ipaddr_t *
bcp_get_nbr_ipaddr(bcp_nbr_t *p)
{
  
  uip_ipaddr_t* ipaddress = NULL;
  rimeaddr_t *lladdr = nbr_table_get_lladdr(bcp_nbrs, p);

  ipaddress = uip_ds6_nbr_ipaddr_from_lladdr((uip_lladdr_t *)lladdr);
  
  return ipaddress;
}


/*****************************************************/
/*               Initialize BCP                      */
/*****************************************************/

void
bcp_init(void)
{
  //printf("bcp.c: BCP is being initialized\n");
  uip_ipaddr_t bcpmaddr;
  PRINTF("BCP started\n");
  bcp_nbr_init();

  /* add bcp multicast address */
  uip_create_linklocal_bcpnodes_mcast(&bcpmaddr);
  uip_ds6_maddr_add(&bcpmaddr);


}


/*****************************************************/
/*               Initialize nbr table                */
/*****************************************************/

void
bcp_nbr_init(void)
{
  //printf("bcp.c: We are initializing the bcp neighbor table\n");
  nbr_table_register(bcp_nbrs, (nbr_table_callback *)bcp_remove_nbr);
}


/*****************************************************/
/*               Remove neighbor                     */
/*****************************************************/

void
bcp_remove_nbr(bcp_nbr_t *nbr)
{
  uip_ipaddr_t *ip = NULL;
  PRINTF("BCP: Removing neighbor ");
  PRINT6ADDR(bcp_get_nbr_ipaddr(nbr));
  PRINTF("\n");
  ip = bcp_get_nbr_ipaddr(nbr);
  
  //printf("Timer: Neighbor timer expired. Remove neighbor %d\n", ip->u8[15]);

  nbr_table_remove(bcp_nbrs, nbr);
}


/*****************************************************/
/*     Add a neighbor to neighbor list               */
/*****************************************************/

bcp_nbr_t *
bcp_add_nbr( bcp_beacon_t *beacon, uip_ipaddr_t *addr)
{
  //printf("bcp.c: we are adding a neighbor over here\n");
  bcp_nbr_t *neighbor = NULL;
 
  uip_lladdr_t *lladdr = uip_ds6_nbr_lladdr_from_ipaddr(addr);
  if(lladdr != NULL) {

    /*Add a neighbor*/
    neighbor = nbr_table_add_lladdr(bcp_nbrs, (rimeaddr_t *)lladdr);
    neighbor->queue_size = beacon->queue_size;   
    neighbor->reserved = beacon->reserved;
    //printf("Timer: Setting timer\n");
    ctimer_set(&neighbor->nbr_timer, 10*CLOCK_SECOND, &handle_nbr_timer, neighbor);
  }


  return neighbor;
}


/*****************************************************/
/*          Process an incoming beacon               */
/*****************************************************/

void
bcp_process_beacon(uip_ipaddr_t *from, bcp_beacon_t *beacon)
{
 
      bcp_nbr_t* nbr = NULL;
  
      if( (nbr = bcp_find_nbr(from)) != NULL){

           nbr->reserved = beacon->reserved;
           nbr->queue_size = beacon->queue_size;  //weight setting other parameters as one
           //printf("Timer: Resetting timer for %d\n", from->u8[15]);
           ctimer_set(&nbr->nbr_timer, 10*CLOCK_SECOND, &handle_nbr_timer, nbr);
         
        }
        else{
         
          bcp_add_nbr(beacon,from);
        }

		    bcp_reset_beacon_timer();    
	  
}


/*****************************************************/
/*          Call back for sending beacon             */
/*****************************************************/
void
handle_bcp_timer()
{
  beacon_output(NULL); 
}



/*****************************************************/
/*     Reset call beacon timer when it expires       */
/*****************************************************/
void
bcp_reset_beacon_timer()
{  
  ctimer_set(&bcp_beacon_timer, CLOCK_SECOND, &handle_bcp_timer, NULL);      
}

/*****************************************************/
/*          Call back for removing neighbor          */
/*****************************************************/

void 
handle_nbr_timer(void* nbr){
  bcp_nbr_t* bcp_nbr = nbr;
  bcp_remove_nbr(bcp_nbr);
  

}
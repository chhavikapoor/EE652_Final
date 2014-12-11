/* 

* This code has been developed as EE652 final project at Viterbi School of Engineering.
* Parts of the code have been adapted from the existing RPL implementation available on Contiki 2.7
* This code shows the basic funcationality  of BCP on IPv6 stack of Contiki

* Authors:
* Chhavi Kapoor ckapoor@usc.edu
* Mrunal Muni muni@usc.edu

*/

#include "net/tcpip.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-nd6.h"
#include "net/uip-icmp6.h"
#include "net/rpl/rpl-private.h"
#include "net/packetbuf.h"
#include "lib/list.h"
#include "bcp.h" 

#include <limits.h>
#include <string.h>

#define DEBUG DEBUG_NONE

#include "net/uip-debug.h"
 extern unsigned short node_id;
 

#if UIP_CONF_IPV6
/*---------------------------------------------------------------------------*/

#define UIP_IP_BUF       ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF     ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_ICMP_PAYLOAD ((unsigned char *)&uip_buf[uip_l2_l3_icmp_hdr_len])
/*---------------------------------------------------------------------------*/



/*****************************************************/
/*            Receive BCP beacon                     */
/*****************************************************/
static void
beacon_input(void)
{
  unsigned char *buffer;
  uint8_t buffer_length;
  bcp_beacon_t beacon;
  uint8_t subopt_type;
  int i;
  int len;
  uip_ipaddr_t from;
  uip_ds6_nbr_t *nbr;

  memset(&beacon, 0, sizeof(beacon)); 
  uip_ipaddr_copy(&from, &UIP_IP_BUF->srcipaddr); //set the from address
  PRINTF("BCP: Received a beacon from ");
  PRINT6ADDR(&from);
  PRINTF("\n");

  if((nbr = uip_ds6_nbr_lookup(&from)) == NULL) {
    if((nbr = uip_ds6_nbr_add(&from, (uip_lladdr_t *)
                              packetbuf_addr(PACKETBUF_ADDR_SENDER),
                              0, NBR_REACHABLE)) != NULL) {
      /* set reachable timer */
      
      PRINTF("BCP: Neighbor added to neighbor cache ");
      PRINT6ADDR(&from);
      PRINTF(", ");
      PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
      PRINTF("\n");
    } else {
      PRINTF("BCP: Out of Memory, dropping beacon from ");
      PRINT6ADDR(&from);
      PRINTF(", ");
      PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
      PRINTF("\n");
      return;
    }
  } else {
    PRINTF("BCP: Neighbor already in neighbor cache\n");
  }

  buffer_length = uip_len - uip_l3_icmp_hdr_len;

  /* Process the BEACON. */
  i = 0;
  buffer = UIP_ICMP_PAYLOAD;
  beacon.queue_size = buffer[i++];   
  beacon.reserved = buffer[i++];

  bcp_process_beacon(&from, &beacon);
}



/*****************************************************/
/*               Send BCP beacon                     */
/*****************************************************/
void
beacon_output(uip_ipaddr_t *uc_addr)
{ //printf("bcp.c: sending out beacon\n");
  unsigned char *buffer;
  uip_ipaddr_t addr;
  int pos;
  

  /* BEACON */   //this is where the beacon is being formed

  pos = 0;

  buffer = UIP_ICMP_PAYLOAD;
  if(node_id == 1){
  printf("BCP: Sending queue size in beacon %d\n", 0);  
  buffer[pos++] = 0;   
  }
  else{
   buffer[pos++] = get_list_length();    
   printf("BCP: Sending queue size in beacon %d\n", buffer[pos -1 ]);
  }
  buffer[pos++] = 1;  //reserved

  uip_create_linklocal_bcpnodes_mcast(&addr);
  uip_icmp6_send(&addr, ICMP6_RPL, BCP_CODE_BEACON, pos);

}


/*****************************************************/
/*      Process BCP control message                  */
/*****************************************************/

void
uip_bcp_input(void)
{

  PRINTF("Received an BCP control message\n");
  switch(UIP_ICMP_BUF->icode) {
  case BCP_CODE_BEACON:
    beacon_input();
    break;
  
  default:
    PRINTF("BCP: received an unknown ICMP6 code (%u)\n", UIP_ICMP_BUF->icode);
    break;
  }

  uip_len = 0;
}
#endif /* UIP_CONF_IPV6 */

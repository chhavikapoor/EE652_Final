/* 

* This code has been developed as EE652 final project at Viterbi School of Engineering.
* Parts of the code have been adapted from the existing RPL implementation available on Contiki 2.7
* This code shows the basic funcationality  of BCP on IPv6 stack of Contiki

* Authors:
* Chhavi Kapoor ckapoor@usc.edu
* Mrunal Muni muni@usc.edu

*/

#include "contiki.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "dev/serial-line.h"
#if CONTIKI_TARGET_Z1
#include "dev/uart0.h"
#else
#include "dev/uart1.h"
#endif
#include "bcp-collect-common.h"
#include "collect-view.h"
#include "list.h"
#include "bcp.h" 


#include <stdio.h>
#include <string.h>

#define UDP_CLIENT_PORT 8775
#define UDP_SERVER_PORT 5688
//#define UDP_SERVER1_PORT 6553
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

extern unsigned short node_id;
static struct uip_udp_conn *server_conn;
static struct uip_udp_conn *client_conn;
int count =0;


/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");   // we are creating a process over here
AUTOSTART_PROCESSES(&udp_client_process, &collect_common_process, &bcp_pop_process); //we are starting the udp client process and the collect common process.
/*---------------------------------------------------------------------------*/
void
bcp_collect_common_set_sink(void)
{
  /* A udp client can never become sink */
}
/*---------------------------------------------------------------------------*/

void
bcp_collect_common_net_print(void)
{
  
}
/*---------------------------------------------------------------------------*/

/*****************************************************/
/*     Handler for incoming packets                  */
/*****************************************************/

static void
bcp_tcpip_handler(void)
{
  uint8_t *appdata;
  rimeaddr_t sender;
  uint8_t seqno;
  uint8_t hops;
  struct collect_view_data_msg *msg;
  struct collect_view_data_msg msg1;

  hdr_information_t hdr_info; 
  bcp_nbr_t *preferred_parent;
  rimeaddr_t parent;


  if(uip_newdata()) {
    appdata = (uint8_t *)uip_appdata;
    sender.u8[0] = UIP_IP_BUF->srcipaddr.u8[15];
    sender.u8[1] = UIP_IP_BUF->srcipaddr.u8[14];
    seqno = *appdata;
    hops = uip_ds6_if.cur_hop_limit - UIP_IP_BUF->ttl + 1;
    
    hdr_info.seqno = seqno;
    hdr_info.hops = UIP_IP_BUF->ttl - 1;  
    hdr_info.sender.u8[0] = sender.u8[0];
    hdr_info.sender.u8[1] = sender.u8[1];
    msg = appdata ;
  
   if(node_id != 1){

   //printf("BCP: Receiving packet from: %d\n", sender.u8[0]); 
   bcp_uip_udp_push_packet(client_conn, msg, sizeof(msg1)+2,
                         UIP_HTONS(UDP_SERVER_PORT),&hdr_info);
  }

  if(node_id == 1){
    bcp_collect_common_recv(&sender, seqno, hops,
                        appdata + 2, uip_datalen() - 2);
   }
 }
  
}
/*---------------------------------------------------------------------------*/



/*****************************************************/
/*     Generate and send packets periodically        */
/*****************************************************/

void
bcp_collect_common_send(void)
{

  if(node_id != 1){
      static uint8_t seqno;
      struct {
        uint8_t seqno;
        uint8_t for_alignment;
        struct collect_view_data_msg msg;
      } msg;
      /* struct collect_neighbor *n; */
      uint16_t nbr_etx;
      uint16_t rtmetric;
      uint16_t num_neighbors;
      uint16_t beacon_interval;
     
      if(client_conn == NULL) {
        /* Not setup yet */
        return;
      }
      memset(&msg, 0, sizeof(msg));
      seqno++;
      if(seqno == 0) {
        /* Wrap to 128 to identify restarts */
        seqno = 128;
      }
      msg.seqno = seqno;
      collect_view_construct_message(&msg.msg, NULL,
                                     nbr_etx, rtmetric,
                                     num_neighbors, beacon_interval);

      bcp_uip_udp_push_packet(client_conn, &msg, sizeof(msg),
                             UIP_HTONS(UDP_SERVER_PORT), NULL);
  }

}
/*---------------------------------------------------------------------------*/
void
bcp_collect_common_net_init(void)
{
#if CONTIKI_TARGET_Z1
  uart0_set_input(serial_line_input_byte);
#else
  uart1_set_input(serial_line_input_byte);
#endif
  serial_line_init();
}
/*---------------------------------------------------------------------------*/
static void
bcp_print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
bcp_set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  /*using node ID to set IP address*/
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, node_id);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);

}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  bcp_set_global_address();

  PRINTF("UDP client process started\n");

  bcp_print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

  /*new connection as server */
  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
        UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));


  bcp_reset_beacon_timer();  


  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      bcp_tcpip_handler();
    }
  }

  PROCESS_END();
}


/*---------------------------------------------------------------------------*/

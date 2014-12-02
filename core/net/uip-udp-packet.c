/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Module for sending UDP packets through uIP.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki-conf.h"

extern uint16_t uip_slen;

#include "net/uip-udp-packet.h"
#include "bcp/bcp.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "collect-view.h"

#include <string.h>

void bcp_uip_udp_packet_sendto(struct uip_udp_conn *c, const void *data, int len, uint16_t toport, hdr_information_t*);
void bcp_uip_udp_packet_send(struct uip_udp_conn *c, const void *data, int len, hdr_information_t*);

#define PERIOD_POP 3
#define RANDWAIT (PERIOD_POP)
int pop_active = 1;
static uip_ipaddr_t server_ipaddr;
uip_ipaddr_t addr;

PROCESS(test_process, "test process");
AUTOSTART_PROCESSES(&test_process);

#define MAX_STACK_LENGTH 15


int init_flag = 0;
int packets_pushed = 0;
typedef struct chhavi_list;
struct chhavi_list{

  struct chhavi_list* next;
  struct uip_udp_conn *c; 
  const void *data; 
  int len;
  
  uint16_t toport;
  struct hdr_information *hdr_info;

};

LIST(mylist);
MEMB(list_memb, struct chhavi_list, MAX_STACK_LENGTH);
MEMB(data_memb, struct collect_view_data_msg, MAX_STACK_LENGTH);
MEMB(hdr_memb, struct hdr_information,MAX_STACK_LENGTH);



/*---------------------------------------------------------------------------*/
void
uip_udp_packet_send(struct uip_udp_conn *c, const void *data, int len)
{


#if UIP_UDP
  if(data != NULL) {
    uip_udp_conn = c;
    uip_slen = len;
    memcpy(&uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN], data,
           len > UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN?
           UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN: len);
    uip_process(UIP_UDP_SEND_CONN);

    //bcp_uip_process();
#if UIP_CONF_IPV6
    //printf("uip-udp-packet.c: Calling tcpip_ipv6_output\n");
    tcpip_ipv6_output();
#else
    if(uip_len > 0) {
      //printf("uip-udp-packet.c: Calling tcpip_output\n");
      tcpip_output();
    }
#endif
  }
  uip_slen = 0;
#endif /* UIP_UDP */
}
/*---------------------------------------------------------------------------*/
void
uip_udp_packet_sendto(struct uip_udp_conn *c, const void *data, int len,
		       const uip_ipaddr_t *toaddr, uint16_t toport)
{
  int length;
  
    if(init_flag == 0){
      //printf("uip-udp-packet.c: initializing the list\n");
      //printf("uip-udp-packet.c: this is the value of the init flag %d\n", init_flag);
      init_flag = 1;
     list_init(mylist);
     memb_init(&list_memb);
     memb_init(&data_memb);
     memb_init(&hdr_memb);
    }
   
   struct chhavi_list *queue_element = (struct chhavi_list*)memb_alloc(&list_memb); 
   struct collect_view_data_msg *temp_msg = (struct collect_view_data_msg*)memb_alloc(&data_memb);
   memcpy(temp_msg,data,len);
   
   queue_element->c = c;
   queue_element->data = temp_msg;
   queue_element->len = len;
   //queue_element->toaddr = toaddr;
   queue_element->toport = toport;
   

   if(packets_pushed < MAX_STACK_LENGTH){
     packets_pushed ++ ;
     //printf("uip-udp-packet.c: We are pushing a packet\n");
     list_push(mylist, queue_element);
     //printf("This is the list length after pushing a packet %d\n", list_length(mylist));
    }

   
}


/*********************************************BCP********************************************/


void
bcp_uip_udp_packet_send(struct uip_udp_conn *c, const void *data, int len, hdr_information_t* hdr_info)
{


#if UIP_UDP
  if(data != NULL) {
    uip_udp_conn = c;
    uip_slen = len;
    memcpy(&uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN], data,
           len > UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN?
           UIP_BUFSIZE - UIP_LLH_LEN - UIP_IPUDPH_LEN: len);
    
    bcp_uip_process(hdr_info);
#if UIP_CONF_IPV6
    //printf("uip-udp-packet.c: Calling tcpip_ipv6_output\n");
    tcpip_ipv6_output();
#else
    if(uip_len > 0) {
      //printf("uip-udp-packet.c: Calling tcpip_output\n");
      tcpip_output();
    }
#endif
  }
  uip_slen = 0;
#endif /* UIP_UDP */
}
/*---------------------------------------------------------------------------*/
void
bcp_uip_udp_packet_sendto(struct uip_udp_conn *c, const void *data, int len,
           uint16_t toport, hdr_information_t* hdr_info)
{
  int length;
  struct hdr_information *temp_hdr_info;
  
    if(init_flag == 0){
      //printf("uip-udp-packet.c: initializing the list\n");
      //printf("uip-udp-packet.c: this is the value of the init flag %d\n", init_flag);
      init_flag = 1;
     list_init(mylist);
    }


   if(packets_pushed < MAX_STACK_LENGTH){
       struct chhavi_list *queue_element = (struct chhavi_list*)memb_alloc(&list_memb); 
       struct collect_view_data_msg *temp_msg = (struct collect_view_data_msg*)memb_alloc(&data_memb);
       memset(temp_msg,0,len);
       memcpy(temp_msg,data,len);

       if(hdr_info!=NULL){
        //printf("Hdr info not null\n");
        temp_hdr_info = (hdr_information_t *)memb_alloc(&hdr_memb);
        memcpy(temp_hdr_info,hdr_info, sizeof( hdr_information_t));
       }
        
       queue_element->c = c;
       queue_element->data = temp_msg;
       queue_element->len = len;
       //queue_element->toaddr = toaddr;
       queue_element->toport = toport;
       if(hdr_info!=NULL){
       queue_element->hdr_info = temp_hdr_info;
       }
       else{
        queue_element->hdr_info = NULL;
       }
             
         packets_pushed ++ ;
         //printf("uip-udp-packet.c: We are pushing a packet\n");
         list_push(mylist, queue_element);
     //printf("This is the list length after pushing a packet %d\n", list_length(mylist));
    }

    else{
      if(hdr_info!=NULL){
        printf("BCP: Dropping packet received from %d as the stack is full\n", hdr_info->sender.u8[0]);
      }
      else{
        printf("BCP: Dropping generated packet as the stack is full\n");
      }
    }

   
}


int get_list_length(){

  int listlength = list_length(mylist);
  
  return listlength;

}





/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)    //this thread is responsible for sending a packet for queuing
{
  static struct etimer period_timer, wait_timer;

  bcp_parent_t *preferred_parent;
  rimeaddr_t parent;

  int length_pop = 0;
  PROCESS_BEGIN();

  //collect_common_net_init();

  /* Send a packet every 60-62 seconds. */
  etimer_set(&period_timer, CLOCK_SECOND * PERIOD_POP);   //setting the period time to the value given by argument 2
  while(1) {
    PROCESS_WAIT_EVENT();                            
    
    if(ev == PROCESS_EVENT_TIMER) {
      //printf("Process timer occurred 1\n");
      if(data == &period_timer) {
        //printf("Process timer occurred 2\n");
        etimer_reset(&period_timer);
        etimer_set(&wait_timer, random_rand() % (CLOCK_SECOND * RANDWAIT));
      } else if(data == &wait_timer) {
          //printf("Process timer occurred 3\n");
          if(pop_active) {
        
       
            length_pop = list_length(mylist);
            //printf("Popping. Queue size is %d\n", length_pop);
            if(length_pop > 0){
             
              rimeaddr_copy(&parent, &rimeaddr_null);
   

          /* Let's suppose we have only one instance */
          
              uip_ds6_nbr_t *nbr;
              preferred_parent = bcp_find_best_parent();
              //printf("This is preferred parent %p\n",preferred_parent);

              if(preferred_parent!=NULL){
                  nbr = uip_ds6_nbr_lookup(bcp_get_parent_ipaddr(preferred_parent));
                  //nbr = uip_ds6_nbr_lookup(&addr);
                  
                  if(nbr != NULL) {
                    //printf("Yay.neighbor is not null\n");
                    /* Use parts of the IPv6 address as the parent address, in reversed byte order. */
                    parent.u8[RIMEADDR_SIZE - 1] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 2];
                    parent.u8[RIMEADDR_SIZE - 2] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1];

                    //printf("rime addr %d:%d\n",parent.u8[RIMEADDR_SIZE - 1],parent.u8[RIMEADDR_SIZE - 2]);
                    //parent_etx = 2;
                  }
          
      
                  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1]) ;

                  struct chhavi_list* temp_element = NULL;
                  //printf("This is the list length. Popping a packet %d\n", length_pop);

                  temp_element = (struct chhavi_list*)list_pop(mylist);
                  //length_pop = list_length(mylist);
                  packets_pushed -- ;

                  if(temp_element!= NULL){

                  //printf("uip-udp-packet.c: We have popped an element and sent it\n");

                        uip_ipaddr_t curaddr;
                        uint16_t curport;

                      //if(temp_element->toaddr != NULL) {
                      /* Save current IP addr/port. */
                        uip_ipaddr_copy(&curaddr, &(temp_element->c)->ripaddr);
                        curport = (temp_element->c)->rport;

                        /* Load new IP addr/port */
                        uip_ipaddr_copy(&(temp_element->c)->ripaddr, &server_ipaddr);
                        (temp_element->c)->rport = temp_element->toport;

                        //printf("This is the ipaddr %d\n", (temp_element->hdr_info)->sender.u8[0]);

                        bcp_uip_udp_packet_send(temp_element->c, temp_element->data, temp_element->len, temp_element->hdr_info);

                        /* Restore old IP addr/port */
                        uip_ipaddr_copy(&(temp_element->c)->ripaddr, &curaddr);       
                        (temp_element->c)->rport = curport;
                       

                        memb_free(&data_memb, temp_element->data );
                        //printf("Memb_free 1\n");
                        //printf("Memb the hdr info ptr %p\n", temp_element->hdr_info);
                        if(temp_element->hdr_info != NULL){
                          memb_free( &hdr_memb, temp_element->hdr_info);
                        }
                        //printf("Memb_free 2\n");
                        memb_free( &list_memb, temp_element);
                        //printf("Memb the element ptr %p\n", temp_element);
                        //printf("Memb_free 3\n");
                 }
          }
        }
      }
    }
  }
}

  PROCESS_END();
}


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

#define PERIOD_POP 1
#define RANDWAIT (PERIOD_POP)
int pop_active = 1;
static uip_ipaddr_t server_ipaddr;
uip_ipaddr_t addr;

PROCESS(bcp_pop_process, "bcp pop process");
AUTOSTART_PROCESSES(&bcp_pop_process);

#define MAX_STACK_LENGTH 25


int init_flag = 0;
int packets_pushed = 0;
typedef struct bcp_list;
struct bcp_list{

  struct bcp_list* next;
  struct uip_udp_conn *c; 
  const void *data; 
  int len;
  uint16_t toport;
  struct hdr_information *hdr_info;

};

LIST(bcplist);
MEMB(list_memb, struct bcp_list, MAX_STACK_LENGTH);
MEMB(data_memb, struct collect_view_data_msg, MAX_STACK_LENGTH);
MEMB(hdr_memb, struct hdr_information,MAX_STACK_LENGTH);


/***************************************BCP********************************************/


/*****************************************************/
/*  Send popped packet for transmission              */
/*****************************************************/


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

/*****************************************************/
/*  Push packets into the stack                      */
/*****************************************************/
void
bcp_uip_udp_packet_sendto(struct uip_udp_conn *c, const void *data, int len,
           uint16_t toport, hdr_information_t* hdr_info)
{
  int length;
  struct hdr_information *temp_hdr_info;
  
  if(init_flag == 0){
      init_flag = 1;
      list_init(bcplist);
      memb_init(&list_memb);
      memb_init(&data_memb);
      memb_init(&hdr_memb);
  }

   if(packets_pushed < MAX_STACK_LENGTH){
       struct bcp_list *queue_element = (struct bcp_list*)memb_alloc(&list_memb); 
       struct collect_view_data_msg *temp_msg = (struct collect_view_data_msg*)memb_alloc(&data_memb);
       memset(temp_msg,0,len);
       memcpy(temp_msg,data,len);

       if(hdr_info!=NULL){
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
       list_push(bcplist, queue_element);
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


/*****************************************************/
/*            Get stack occupancy                    */
/*****************************************************/

int get_list_length(){

  int listlength = list_length(bcplist);
  return listlength;

}


/*****************************************************/
/*   Pop packets from the stack                      */
/*****************************************************/

PROCESS_THREAD(bcp_pop_process, ev, data)    
{
  static struct etimer period_timer, wait_timer;
  bcp_nbr_t *preferred_parent;
  rimeaddr_t parent;
  int length_pop = 0;

  PROCESS_BEGIN();

  //setting the period time to the value given by argument 2
  etimer_set(&period_timer, (CLOCK_SECOND * PERIOD_POP));   
  while(1) {
  PROCESS_WAIT_EVENT();                            
    
    if(ev == PROCESS_EVENT_TIMER) {
      if(data == &period_timer) {
        etimer_reset(&period_timer);
        etimer_set(&wait_timer, random_rand() % (CLOCK_SECOND * RANDWAIT));
      } 
      else if(data == &wait_timer) {
        
          if(pop_active) {
        
            length_pop = list_length(bcplist);
            if(length_pop > 0){
              rimeaddr_copy(&parent, &rimeaddr_null);
              uip_ds6_nbr_t *nbr;
              preferred_parent = bcp_find_best_parent();
              if(preferred_parent!=NULL){
                  nbr = uip_ds6_nbr_lookup(bcp_get_nbr_ipaddr(preferred_parent));
                  if(nbr != NULL) {
                    /* Use parts of the IPv6 address as the parent address, in reversed byte order. */
                    parent.u8[RIMEADDR_SIZE - 1] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 2];
                    parent.u8[RIMEADDR_SIZE - 2] = nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1];

                  }

                  uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, nbr->ipaddr.u8[sizeof(uip_ipaddr_t) - 1]) ;
                  struct bcp_list* temp_element = NULL;
                  temp_element = (struct bcp_list*)list_pop(bcplist);
                  packets_pushed -- ;

                  if(temp_element!= NULL){
                    uip_ipaddr_t curaddr;
                    uint16_t curport;

                    //if(temp_element->toaddr != NULL) {
                    /* Save current IP addr/port. */
                    uip_ipaddr_copy(&curaddr, &(temp_element->c)->ripaddr);
                    curport = (temp_element->c)->rport;

                    /* Load new IP addr/port */
                    uip_ipaddr_copy(&(temp_element->c)->ripaddr, &server_ipaddr);
                    (temp_element->c)->rport = temp_element->toport;
                    bcp_uip_udp_packet_send(temp_element->c, temp_element->data, temp_element->len, temp_element->hdr_info);

                    /* Restore old IP addr/port */
                    uip_ipaddr_copy(&(temp_element->c)->ripaddr, &curaddr);       
                    (temp_element->c)->rport = curport;
                   
                    memb_free(&data_memb, temp_element->data );
                    if(temp_element->hdr_info != NULL){
                      memb_free( &hdr_memb, temp_element->hdr_info);
                    }
                    memb_free( &list_memb, temp_element);
                     
                 }
          }
        }
      }
    }
  }
}

  PROCESS_END();
}



/*----------------------------------------------------------------------------*/

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
      #if UIP_CONF_IPV6
      tcpip_ipv6_output();
      #else
      if(uip_len > 0) {
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
  uip_ipaddr_t curaddr;
  uint16_t curport;
  if(toaddr != NULL) {
  /* Save current IP addr/port. */
    uip_ipaddr_copy(&curaddr, &c->ripaddr);
    curport = c->rport;
    /* Load new IP addr/port */
    uip_ipaddr_copy(&c->ripaddr, toaddr);
    c->rport = toport;
    uip_udp_packet_send(c, data, len);
    /* Restore old IP addr/port */
    uip_ipaddr_copy(&c->ripaddr, &curaddr);
    c->rport = curport;
  }
}


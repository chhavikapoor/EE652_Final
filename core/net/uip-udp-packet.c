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
#include "rpl/bcp.h"
#include "lib/list.h"
#include "collect-view.h"

#include <string.h>

#define PERIOD_POP 1
#define RANDWAIT (PERIOD_POP)
int pop_active = 1;

PROCESS(test_process, "test process");
AUTOSTART_PROCESSES(&test_process);

#define MAX_STACK_LENGTH 5


int init_flag = 0;
int packets_pushed = 0;
typedef struct chhavi_list;
struct chhavi_list{

  struct chhavi_list* next;
  struct uip_udp_conn *c; 
  const void *data; 
  int len;
  const uip_ipaddr_t *toaddr;
  uint16_t toport;

};

LIST(mylist);



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
#if UIP_CONF_IPV6
    printf("uip-udp-packet.c: Calling tcpip_ipv6_output\n");
    tcpip_ipv6_output();
#else
    if(uip_len > 0) {
      printf("uip-udp-packet.c: Calling tcpip_output\n");
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
      printf("uip-udp-packet.c: initializing the list\n");
      //printf("uip-udp-packet.c: this is the value of the init flag %d\n", init_flag);
      init_flag = 1;
     list_init(mylist);
    }
   
   struct chhavi_list *queue_element = (struct chhavi_list*)malloc(sizeof(struct chhavi_list)); 
   struct collect_view_data_msg *temp_msg = (struct collect_view_data_msg*)malloc(sizeof(struct collect_view_data_msg));
   memcpy(temp_msg,data,len);
    
   queue_element->c = c;
   queue_element->data = temp_msg;
   queue_element->len = len;
   queue_element->toaddr = toaddr;
   queue_element->toport = toport;


   if(packets_pushed < MAX_STACK_LENGTH){
     packets_pushed ++ ;
     printf("uip-udp-packet.c: We are pushing a packet\n");
     list_push(mylist, queue_element);
     //printf("This is the list length after pushing a packet %d\n", list_length(mylist));
    }

   
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)    //this thread is responsible for sending a packet for queuing
{
  static struct etimer period_timer, wait_timer;
  int length_pop = 0;
  PROCESS_BEGIN();

  //collect_common_net_init();

  /* Send a packet every 60-62 seconds. */
  etimer_set(&period_timer, CLOCK_SECOND * PERIOD_POP);   //setting the period time to the value given by argument 2
  while(1) {
    PROCESS_WAIT_EVENT();                            
    
    if(ev == PROCESS_EVENT_TIMER) {
      if(data == &period_timer) {
        etimer_reset(&period_timer);
        etimer_set(&wait_timer, random_rand() % (CLOCK_SECOND * RANDWAIT));
      } else if(data == &wait_timer) {
          if(pop_active) {
            /* Time to send the data */
            //printf("Hey this is a new thread. We are popping elements\n");
       
            length_pop = list_length(mylist);
            //printf("this is the length of the list %d\n", length_pop);
            if(length_pop > 0){
            struct chhavi_list* temp_element = NULL;
            //printf("This is the list length. Popping a packet %d\n", length_pop);

            temp_element = (struct chhavi_list*)list_pop(mylist);
            //length_pop = list_length(mylist);
            packets_pushed -- ;

            if(temp_element!= NULL){

              printf("uip-udp-packet.c: We have popped an element and sent it\n");

              uip_ipaddr_t curaddr;
              uint16_t curport;

              if(temp_element->toaddr != NULL) {
              /* Save current IP addr/port. */
                uip_ipaddr_copy(&curaddr, &(temp_element->c)->ripaddr);
                curport = (temp_element->c)->rport;

                /* Load new IP addr/port */
                uip_ipaddr_copy(&(temp_element->c)->ripaddr, temp_element->toaddr);
                (temp_element->c)->rport = temp_element->toport;

                uip_udp_packet_send(temp_element->c, temp_element->data, temp_element->len);

                /* Restore old IP addr/port */
                uip_ipaddr_copy(&(temp_element->c)->ripaddr, &curaddr);       
                (temp_element->c)->rport = curport;
                //free(temp_element->data);
                //free(temp_element);
              }
            }
          }
        }
      }
    }
  }

  PROCESS_END();
}


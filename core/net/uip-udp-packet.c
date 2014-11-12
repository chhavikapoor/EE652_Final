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
#include "lib/list.h"

#include <string.h>

#define MAX_STACK_LENGTH 1


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
    printf("Calling tcpip_ipv6_output\n");
    tcpip_ipv6_output();
#else
    if(uip_len > 0) {
      printf("Calling tcpip_output\n");
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
  printf("initializing the list\n");
  printf("this is the value of the init flag %d\n", init_flag);
    if(init_flag == 0){
      init_flag = 1;
     list_init(mylist);
    }
   
   struct chhavi_list *queue_element = (struct chhavi_list*)malloc(sizeof(struct chhavi_list)); 
    
   queue_element->c = c;
   queue_element->data = data;
   queue_element->len = len;
   queue_element->toaddr = toaddr;
   queue_element->toport = toport;


   if(packets_pushed < MAX_STACK_LENGTH){
     packets_pushed ++ ;
     list_push(mylist, queue_element);
     printf("This is the list length after pushing a packet %d\n", list_length(mylist));
    }

   else{ 
    length = list_length(mylist);
      printf("this is the length of the list %d\n", length);
      while(length > 0){
        struct chhavi_list* temp_element = NULL;
        printf("This is the list length. Popping a packet %d\n", length);

        temp_element = (struct chhavi_list*)list_pop(mylist);
        length = list_length(mylist);
        packets_pushed -- ;

        if(temp_element!= NULL){

          uip_ipaddr_t curaddr;
          uint16_t curport;

          if(temp_element->toaddr != NULL) {
          /* Save current IP addr/port. */
            uip_ipaddr_copy(&curaddr, &(temp_element->c)->ripaddr);
            curport = (temp_element->c)->rport;

            /* Load new IP addr/port */
            uip_ipaddr_copy(&(temp_element->c)->ripaddr, temp_element->toaddr);
            (temp_element->c)->rport = temp_element->toport;

            uip_udp_packet_send(temp_element->c, data, temp_element->len);

            /* Restore old IP addr/port */
            uip_ipaddr_copy(&(temp_element->c)->ripaddr, &curaddr);       
            (temp_element->c)->rport = curport;
          }
        }
      }
  }
}
/*---------------------------------------------------------------------------*/

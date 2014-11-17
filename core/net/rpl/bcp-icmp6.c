/**
 * \addtogroup uip6
 * @{
 */
/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *         ICMP6 I/O for RPL control messages.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 * Contributors: Niclas Finne <nfi@sics.se>, Joel Hoglund <joel@sics.se>,
 *               Mathieu Pouillot <m.pouillot@watteco.com>
 */

#include "net/tcpip.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-nd6.h"
#include "net/uip-icmp6.h"
#include "net/rpl/rpl-private.h"
#include "net/packetbuf.h"
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

static void beacon_input(void);
//static uint8_t dao_sequence = RPL_LOLLIPOP_INIT;
//extern rpl_of_t RPL_OF;



/*---------------------------------------------------------------------------*/
static uint32_t
get32(uint8_t *buffer, int pos)
{
  return (uint32_t)buffer[pos] << 24 | (uint32_t)buffer[pos + 1] << 16 |
         (uint32_t)buffer[pos + 2] << 8 | buffer[pos + 3];
}
/*---------------------------------------------------------------------------*/
static void
set32(uint8_t *buffer, int pos, uint32_t value)
{
  buffer[pos++] = value >> 24;
  buffer[pos++] = (value >> 16) & 0xff;
  buffer[pos++] = (value >> 8) & 0xff;
  buffer[pos++] = value & 0xff;
}
/*---------------------------------------------------------------------------*/
static uint16_t
get16(uint8_t *buffer, int pos)
{
  return (uint16_t)buffer[pos] << 8 | buffer[pos + 1];
}
/*---------------------------------------------------------------------------*/
static void
set16(uint8_t *buffer, int pos, uint16_t value)
{
  buffer[pos++] = value >> 8;
  buffer[pos++] = value & 0xff;
}
/*---------------------------------------------------------------------------*/

static void
beacon_input(void)
{
  unsigned char *buffer;
  uint8_t buffer_length;
  rpl_dio_t dio;
  uint8_t subopt_type;
  int i;
  int len;
  uip_ipaddr_t from;
  uip_ds6_nbr_t *nbr;

  memset(&dio, 0, sizeof(dio));

    
  uip_ipaddr_copy(&from, &UIP_IP_BUF->srcipaddr); //set the from address

  /* DAG Information Object */
  PRINTF("RPL: Received a DIO from ");
  PRINT6ADDR(&from);
  PRINTF("\n");

  if((nbr = uip_ds6_nbr_lookup(&from)) == NULL) {
    if((nbr = uip_ds6_nbr_add(&from, (uip_lladdr_t *)
                              packetbuf_addr(PACKETBUF_ADDR_SENDER),
                              0, NBR_REACHABLE)) != NULL) {
      /* set reachable timer */
      stimer_set(&nbr->reachable, UIP_ND6_REACHABLE_TIME / 1000);
      PRINTF("RPL: Neighbor added to neighbor cache ");
      PRINT6ADDR(&from);
      PRINTF(", ");
      PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
      PRINTF("\n");
    } else {
      PRINTF("RPL: Out of Memory, dropping DIO from ");
      PRINT6ADDR(&from);
      PRINTF(", ");
      PRINTLLADDR((uip_lladdr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
      PRINTF("\n");
      return;
    }
  } else {
    PRINTF("RPL: Neighbor already in neighbor cache\n");
  }

  buffer_length = uip_len - uip_l3_icmp_hdr_len;

  /* Process the DIO base option. */
  i = 0;
  buffer = UIP_ICMP_PAYLOAD;
  dio.queue_size = buffer[i++];   //added by chhavi

  /*dio.prefix_info.length = buffer[i + 2];
  dio.prefix_info.flags = buffer[i + 3];
  dio.prefix_info.lifetime = get32(buffer, i + 8);
  PRINTF("RPL: Copying prefix information\n");
  memcpy(&dio.prefix_info.prefix, &buffer[i + 16], 16);*/
  bcp_process_beacon(&from, &dio);
}
/*---------------------------------------------------------------------------*/
void
beacon_output(uip_ipaddr_t *uc_addr)
{ printf("bcp.c: sending out beacon\n");
  unsigned char *buffer;
  int pos;
  

  /* DAG Information Object */   //this is where the DIO is being formed

  //hardcoding the prefix information for the time being

  pos = 0;

  buffer = UIP_ICMP_PAYLOAD;
  buffer[pos++] = node_id;   //added by chhavi  
  /*buffer[pos++] = RPL_OPTION_PREFIX_INFO;
  //buffer[pos++] = 30; // always 30 bytes + 2 long 
  buffer[pos++] = dag->prefix_info.length;
  buffer[pos++] = dag->prefix_info.flags;
  set32(buffer, pos, dag->prefix_info.lifetime);
  pos += 4;
  set32(buffer, pos, dag->prefix_info.lifetime);
  pos += 4;
  memset(&buffer[pos], 0, 4);
  pos += 4;
  memcpy(&buffer[pos], &dag->prefix_info.prefix, 16);
  pos += 16;
  PRINTF("RPL: Sending prefix info in DIO for ");
  PRINT6ADDR(&dag->prefix_info.prefix);
  PRINTF("\n");*/
  uip_icmp6_send(uc_addr, ICMP6_RPL, RPL_CODE_DIO, pos);

}
/*---------------------------------------------------------------------------*/

void
uip_bcp_input(void)
{

   //beacon_input();
  PRINTF("Received an RPL control message\n");
  switch(UIP_ICMP_BUF->icode) {
  case RPL_CODE_DIO:
    beacon_input();
    break;
  /*case RPL_CODE_DIS:
    dis_input();
    break;
  case RPL_CODE_DAO:
    dao_input();
    break;
  case RPL_CODE_DAO_ACK:
    dao_ack_input();
    break;*/
  default:
    PRINTF("RPL: received an unknown ICMP6 code (%u)\n", UIP_ICMP_BUF->icode);
    break;
  }

  uip_len = 0;
}
#endif /* UIP_CONF_IPV6 */

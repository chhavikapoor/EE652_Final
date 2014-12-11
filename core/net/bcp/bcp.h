/* 

* This code has been developed as EE652 final project at Viterbi School of Engineering.
* Parts of the code have been adapted from the existing RPL implementation available on Contiki 2.7
* This code shows the basic funcationality  of BCP on IPv6 stack of Contiki

* Authors:
* Chhavi Kapoor ckapoor@usc.edu
* Mrunal Muni muni@usc.edu

*/


#include "net/rpl/rpl-private.h"


#define BCP_CODE_BEACON 1
#define uip_create_linklocal_bcpnodes_mcast(addr) \
  uip_ip6addr((addr), 0xff02, 0, 0, 0, 0, 0, 0, 0x001a)


struct bcp_nbr {
  struct bcp_nbr *next;
  uint8_t queue_size;
  uint8_t reserved;
  struct ctimer nbr_timer;
};
typedef struct bcp_nbr bcp_nbr_t;

struct bcp_beacon {
  uint8_t queue_size;
  uint8_t reserved;	
};
typedef struct bcp_beacon bcp_beacon_t;

struct hdr_information {
  rimeaddr_t sender;
  uint8_t seqno;
  uint8_t hops;  

};
typedef struct hdr_information hdr_information_t;

void bcp_init(void);
void bcp_nbr_init(void);
void bcp_process_beacon(uip_ipaddr_t *from, bcp_beacon_t *beacon);
void handle_nbr_timer(void* nbr);
void bcp_remove_nbr(bcp_nbr_t *nbr);
void handle_bcp_timer();
void bcp_reset_beacon_timer();
int get_list_length();
uip_ipaddr_t *bcp_get_nbr_ipaddr(bcp_nbr_t *n);
bcp_nbr_t * bcp_find_nbr(uip_ipaddr_t *addr);
bcp_nbr_t * bcp_find_next_hop();
bcp_nbr_t * bcp_add_nbr( bcp_beacon_t *beacon, uip_ipaddr_t *addr);

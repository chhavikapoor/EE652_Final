#include "net/rpl/rpl-private.h"


struct bcp_parent {
  struct bcp_parent *next;
  uint16_t link_metric;
  uint8_t dtsn;
  uint8_t updated;
};
typedef struct bcp_parent bcp_parent_t;



struct bcp_beacon {
  
  //rpl_prefix_t prefix_info;
  uint8_t queue_size;
};


struct hdr_information {
  rimeaddr_t sender;
  uint8_t seqno;
  uint8_t hops;  

};
typedef struct hdr_information hdr_information_t;


typedef struct bcp_beacon bcp_beacon_t;



uip_ipaddr_t *bcp_get_parent_ipaddr(bcp_parent_t *p);
void bcp_init(void);
void bcp_reset_periodic_timer(void);
void bcp_handle_periodic_timer(void *ptr);
void bcp_nbr_init(void);
void bcp_process_beacon(uip_ipaddr_t *from, rpl_dio_t *dio);
void handle_dio_timer(void *ptr);
void bcp_remove_parent(bcp_parent_t *parent);
bcp_parent_t * bcp_add_parent( rpl_dio_t *dio, uip_ipaddr_t *addr);
void bcp_process_beacon(uip_ipaddr_t *from, rpl_dio_t *dio);
void handle_bcp_timer();
void bcp_reset_beacon_timer();

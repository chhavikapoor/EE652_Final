#include "packetstack.h"

/*---------------------------------------------------------------------------*/
void packetstack_init(struct packetstack *s)
{
  list_init(*s->list);
  memb_init(s->memb);
}
/*---------------------------------------------------------------------------*/
struct packetstack_item *
packetstack_push_packetbuf(struct packetstack *s, void *ptr, uint16_t *p)
{
  struct packetstack_item *i;
  // Allocate a memory block to hold the packet stack item
  i = memb_alloc(s->memb);

  if(i == NULL) 
{
	packetstack_remove(s,list_tail(*s->list));
	*p = 1;
	RE:i=memb_alloc(s->memb);
}

  // Allocate a queuebuf and copy the contents of the packetbuf into it
i->buf = queuebuf_new_from_packetbuf();

  if(i->buf == NULL)
{
	packetstack_remove(s,list_tail(*s->list));
	*p = 1;
	goto RE;
}

  i->stack = s;
  i->ptr = ptr;

  // Add the item to the stack
  list_push(*s->list, i);
  //printf("list length = %d\n",list_length(*s->list));
  return packetstack_top(s);
}
/*---------------------------------------------------------------------------*/
struct packetstack_item *
packetstack_top(struct packetstack *s)
{
  return list_head(*s->list);
}
/*---------------------------------------------------------------------------*/
void packetstack_pop(struct packetstack *s)
{
  struct packetstack_item *i;

  i = list_head(*s->list);
  if(i != NULL) {
    list_pop(*s->list);
    queuebuf_free(i->buf);
    memb_free(s->memb, i);
  }
}
/*---------------------------------------------------------------------------*/
void packetstack_remove(struct packetstack *s, struct packetstack_item *i)
{
  if(i != NULL) {
    list_remove(*s->list, i);
    queuebuf_free(i->buf);
    memb_free(s->memb, i);
  }
}
/*---------------------------------------------------------------------------*/
int packetstack_len(struct packetstack *s)
{
  return list_length(*s->list);
}
/*---------------------------------------------------------------------------*/
struct queuebuf *
packetstack_queuebuf(struct packetstack_item *i)
{
  if(i != NULL) {
    return i->buf;
  } else {
    return NULL;
  }
}

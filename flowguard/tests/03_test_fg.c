#include <stdint.h>
#include <string.h>

typedef unsigned long size_t;
void resolv_query(char const *name);

enum __anonenum_2 {
    MDNS_STATE_WAIT_BEFORE_PROBE = 0,
    MDNS_STATE_PROBING = 1,
    MDNS_STATE_READY = 2
};

struct namemap {
   uint8_t state ;
   uint8_t tmr ;
   uint16_t id ;
   uint8_t retries ;
   uint8_t seqno ;
   unsigned long expiration ;
   uint8_t err ;
   uint8_t server ;
   int is_mdns : 1 ;
   int is_probe : 1 ;
   char name[32 + 1] ;
};

static struct namemap names[4];
static uint8_t seqno;
static uint8_t mdns_state;
static char resolv_hostname[32 + 1];

// INSIDE A FUNCTION
//resolv_query((char const *)(resolv_hostname));


//void resolv_query(char const *name)
//{
//  //int tmp_4;
//  register struct namemap *nameptr = (struct namemap *)0;
//  
//  nameptr->is_probe = 1;
//  return;
//}


void resolv_query(char const *name)
{
  uint8_t i;
  uint8_t lseq;
  uint8_t lseqi;
  int tmp_4;
  register struct namemap *nameptr = (struct namemap *)0;
  init_1();
  lseqi = (unsigned char)0;
  lseq = lseqi;
  name = remove_trailing_dots(name);
  i = (unsigned char)0;
  while ((int)i < 4) {
    {
      int tmp;
      nameptr = & names[i];
      tmp = strcasecmp((char const *)(nameptr->name),name);
      if (0 == tmp) break;
      if ((int)nameptr->state == 0) goto _LOR;
      else 
        if ((int)nameptr->state == 4) {
          unsigned long tmp_0;
          tmp_0 = clock_seconds();
          ;
          if (tmp_0 > nameptr->expiration) {
            _LOR: {
                    lseqi = i;
                    lseq = (unsigned char)255;
                  }
          }
          else goto _LAND;
        }
        else {
          _LAND: ;
          if ((int)seqno - (int)nameptr->seqno > (int)lseq) {
            lseq = (unsigned char)((int)seqno - (int)nameptr->seqno);
            lseqi = i;
          }
        }
    }
    i = (uint8_t)((int)i + 1);
  }
  if ((int)i == 4) {
    i = lseqi;
    nameptr = & names[i];
  }
  memset((void *)nameptr,0,sizeof(*nameptr));
  strncpy(nameptr->name,name,sizeof(nameptr->name) - (unsigned long)1);
  nameptr->state = (unsigned char)2;
  nameptr->seqno = seqno;
  seqno = (uint8_t)((int)seqno + 1);
  {
    size_t name_len = strlen(name);
    char const local_suffix[6] =
      {(char)'l', (char)'o', (char)'c', (char)'a', (char)'l', (char)'\000'};
    if (name_len > sizeof(local_suffix) - (unsigned long)1) {
      int tmp_2;
      tmp_2 = strcasecmp((name + name_len) - (sizeof(local_suffix) - (unsigned long)1),
                         local_suffix);
      if (0 == tmp_2) nameptr->is_mdns = (int)1; else goto _LAND_0;
    }
    else _LAND_0: nameptr->is_mdns = (int)0;
  }

  if ((int)mdns_state == 1) {
    int tmp_3;
    tmp_3 = strcmp((char const *)(nameptr->name),
                   (char const *)(resolv_hostname));
    tmp_3 = 0;
    if (0 == tmp_3)
        tmp_4 = 1;
    else
        tmp_4 = 0;
  }
  else 
      tmp_4 = 0;
  nameptr->is_probe = (int)tmp_4;
  return;
}

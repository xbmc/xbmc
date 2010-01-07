#if !defined(_NAVIGATION_H_)
#define _NAVIGATION_H_

#include <stdint.h>
#include "mpls_parse.h"
#include "clpi_parse.h"

#define CONNECT_NON_SEAMLESS 0
#define CONNECT_SEAMLESS 1

typedef struct {
    int      number;
    int      clip_ref;
    uint32_t clip_pkt;

    // Title relative metrics
    uint32_t title_pkt;
    uint32_t title_time;
    uint32_t duration;

    MPLS_PLM *plm;
} NAV_CHAP;

typedef struct {
    int      count;
    NAV_CHAP *chapter;
} NAV_CHAP_LIST;

typedef struct {
    char     name[11];
    int      ref;
    uint32_t start_pkt;
    uint32_t end_pkt;
    uint8_t  connection;
    uint8_t  angle;

    // Title relative metrics
    uint32_t title_pkt;
    uint32_t title_time;
    uint32_t duration;

    CLPI_CL  *cl;
} NAV_CLIP;

typedef struct {
    int      count;
    NAV_CLIP *clip;
} NAV_CLIP_LIST;

typedef struct {
    char          root[1024];
    char          name[11];
    uint8_t       angle;
    NAV_CLIP_LIST clip_list;
    NAV_CHAP_LIST chap_list;

    uint32_t      packets;
    uint32_t      duration;

    MPLS_PL       *pl;
} NAV_TITLE;

char* nav_find_main_title(char *root);
NAV_TITLE* nav_title_open(char *root, char *playlist);
void nav_title_close(NAV_TITLE *title);
NAV_CLIP* nav_next_clip(NAV_TITLE *title, NAV_CLIP *clip);
NAV_CLIP* nav_packet_search(NAV_TITLE *title, uint32_t pkt, uint32_t *out_pkt, uint32_t *out_time);
NAV_CLIP* nav_time_search(NAV_TITLE *title, uint32_t tick, uint32_t *out_pkt);
NAV_CLIP* nav_chapter_search(NAV_TITLE *title, int chapter, uint32_t *out_pkt);
uint32_t nav_angle_change_search(NAV_CLIP *clip, uint32_t pkt, uint32_t *time);
NAV_CLIP* nav_set_angle(NAV_TITLE *title, NAV_CLIP *clip, int angle);

#endif // _NAVIGATION_H_

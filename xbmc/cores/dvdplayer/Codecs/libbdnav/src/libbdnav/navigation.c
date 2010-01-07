#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdint.h>
#include <string.h>
#include "../util/macro.h"
#include "../util/logging.h"
#include "../util/strutl.h"
#include "../file/dir.h"
#include "../file/file.h"
#include "mpls_parse.h"
#include "navigation.h"

static int _filter_dup(MPLS_PL *pl_list[], int count, MPLS_PL *pl)
{
    int ii, jj;

    for (ii = 0; ii < count; ii++) {
        if (pl->list_count != pl_list[ii]->list_count) {
            continue;
        }
        for (jj = 0; jj < pl->list_count; jj++) {
            MPLS_PI *pi1, *pi2;

            pi1 = &pl->play_item[jj];
            pi2 = &pl_list[ii]->play_item[jj];

            if (memcmp(pi1->clip[0].clip_id, pi2->clip[0].clip_id, 5) != 0 ||
                pi1->in_time != pi2->in_time ||
                pi1->out_time != pi2->out_time) {
                break;
            }
        }
        if (jj != pl->list_count) {
            continue;
        }
        return 0;
    }
    return 1;
}

static uint32_t
_pl_duration(MPLS_PL *pl)
{
    int ii;
    uint32_t duration = 0;
    MPLS_PI *pi;

    for (ii = 0; ii < pl->list_count; ii++) {
        pi = &pl->play_item[ii];
        duration += pi->out_time - pi->in_time;
    }
    return duration;
}

char* nav_find_main_title(char *root)
{
    DIR_H *dir;
    DIRENT ent;
    char *path = NULL;
    MPLS_PL **pl_list = NULL;
    MPLS_PL **tmp = NULL;
    MPLS_PL *pl = NULL;
    int count, ii, jj, pl_list_size = 0;
    int res;
    char longest[11];

    //DEBUG(DBG_NAV, "Root: %s:\n", root);
    path = str_printf("%s" DIR_SEP "BDMV" DIR_SEP "PLAYLIST", root);
    if (path == NULL) {
        fprintf(stderr, "Failed to find playlist path: %s\n", path);
        return NULL;
    }

    dir = dir_open(path);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open dir: %s\n", path);
        X_FREE(path);
        return NULL;;
    }
    X_FREE(path);

    ii = jj = 0;
    for (res = dir_read(dir, &ent); !res; res = dir_read(dir, &ent)) {

        if (ent.d_name[0] == '.') {
            continue;
        }
        path = str_printf("%s" DIR_SEP "BDMV" DIR_SEP "PLAYLIST" DIR_SEP "%s",
                          root, ent.d_name);

        if (ii >= pl_list_size) {
            pl_list_size += 100;
            tmp = realloc(pl_list, pl_list_size * sizeof(MPLS_PL*));
            if (tmp == NULL) {
                break;
            }
            pl_list = tmp;
        }
        pl = mpls_parse(path, 0);
        X_FREE(path);
        if (pl != NULL) {
            if (_filter_dup(pl_list, ii, pl)) {
                pl_list[ii] = pl;
                if (_pl_duration(pl_list[ii]) >= _pl_duration(pl_list[jj])) {
                    strncpy(longest, ent.d_name, 11);
                    longest[10] = '\0';
                    jj = ii;
                }
                ii++;
            } else {
                mpls_free(pl);
            }
        }
    }
    dir_close(dir);

    count = ii;
    for (ii = 0; ii < count; ii++) {
        mpls_free(pl_list[ii]);
    }
    if (count > 0) {
        return strdup(longest);
    } else {
        return NULL;
    }
}

static void
_extrapolate(NAV_TITLE *title)
{
    uint64_t duration = 0;
    uint64_t pkt = 0;
    int ii, jj;
    MPLS_PL *pl = title->pl;
    MPLS_PI *pi;
    MPLS_PLM *plm;
    NAV_CHAP *chap, *prev = NULL;
    NAV_CLIP *clip;

    for (ii = 0; ii < title->clip_list.count; ii++) {
        clip = &title->clip_list.clip[ii];
        pi = &pl->play_item[ii];

        clip->title_time = duration;
        clip->duration = pi->out_time - pi->in_time;
        clip->title_pkt = pkt;
        duration += clip->duration;
        pkt += clip->end_pkt - clip->start_pkt;
    }
    title->duration = duration;
    title->packets = pkt;

    for (ii = 0, jj = 0; ii < pl->mark_count; ii++) {
        plm = &pl->play_mark[ii];
        if (plm->mark_type == BD_MARK_ENTRY) {

            chap = &title->chap_list.chapter[jj];

            chap->number = jj;
            chap->plm = plm;
            chap->clip_ref = plm->play_item_ref;
            clip = &title->clip_list.clip[chap->clip_ref];
            chap->clip_pkt = clpi_lookup_spn(clip->cl, plm->time, 1,
                title->pl->play_item[chap->clip_ref].clip[title->angle].stc_id);

            // Calculate start of mark relative to beginning of playlist
            if (plm->play_item_ref < title->clip_list.count) {
                clip = &title->clip_list.clip[plm->play_item_ref];
                pi = &pl->play_item[plm->play_item_ref];
                chap->title_time = clip->title_time + plm->time - pi->in_time;
            } else {
                // Invalid chapter mark
                continue;
            }

            // Calculate duration of "entry" marks (chapters)
            if (plm->duration != 0) {
                chap->duration = plm->duration;
            } else if (prev != NULL) {
                if (prev->duration == 0) {
                    prev->duration = chap->title_time - prev->title_time;
                }
            }
            prev = chap;
            jj++;
        }
    }
    title->chap_list.count = jj;
    if (prev->duration == 0) {
        prev->duration = title->duration - prev->title_time;
    }
}

NAV_TITLE* nav_title_open(char *root, char *playlist)
{
    NAV_TITLE *title = NULL;
    char *path;
    int ii, chapters;

    title = malloc(sizeof(NAV_TITLE));
    if (title == NULL) {
        return NULL;
    }
    strncpy(title->root, root, 1024);
    title->root[1023] = '\0';
    strncpy(title->name, playlist, 11);
    title->name[10] = '\0';
    path = str_printf("%s" DIR_SEP "BDMV" DIR_SEP "PLAYLIST" DIR_SEP "%s",
                      root, playlist);
    title->angle = 0;
    title->pl = mpls_parse(path, 0);
    if (title->pl == NULL) {
        DEBUG(DBG_NAV, "Fail: Playlist parse %s\n", path);
        X_FREE(title);
        X_FREE(path);
        return NULL;
    }
    X_FREE(path);
    // Find length in packets and end_pkt for each clip
    title->clip_list.count = title->pl->list_count;
    title->clip_list.clip = malloc(title->pl->list_count * sizeof(NAV_CLIP));
    title->packets = 0;
    for (ii = 0; ii < title->pl->list_count; ii++) {
        MPLS_PI *pi;
        NAV_CLIP *clip;

        pi = &title->pl->play_item[ii];

        clip = &title->clip_list.clip[ii];
        clip->ref = ii;
        clip->angle = 0;
        strncpy(clip->name, pi->clip[clip->angle].clip_id, 5);
        strncpy(&clip->name[5], ".m2ts", 6);

        path = str_printf("%s"DIR_SEP"BDMV"DIR_SEP"CLIPINF"DIR_SEP"%s.clpi",
                      title->root, pi->clip[clip->angle].clip_id);
        clip->cl = clpi_parse(path, 0);
        X_FREE(path);
        if (clip->cl == NULL) {
            clip->start_pkt = 0;
            clip->end_pkt = 0;
            continue;
        }
        switch (pi->connection_condition) {
            case 5:
            case 6:
                clip->start_pkt = 0;
                clip->connection = CONNECT_SEAMLESS;
                break;
            default:
                clip->start_pkt = clpi_lookup_spn(clip->cl, pi->in_time, 1,
                                                  pi->clip[clip->angle].stc_id);
                clip->connection = CONNECT_NON_SEAMLESS;
            break;
        }
        clip->end_pkt = clpi_lookup_spn(clip->cl, pi->out_time, 0,
                                        pi->clip[clip->angle].stc_id);
    }
    // Count the number of "entry" marks (skipping "link" marks)
    // This is the the number of chapters
    for (ii = 0; ii < title->pl->mark_count; ii++) {
        if (title->pl->play_mark[ii].mark_type == BD_MARK_ENTRY) {
            chapters++;
        }
    }
    title->chap_list.count = chapters;
    title->chap_list.chapter = calloc(chapters, sizeof(NAV_CHAP));

    _extrapolate(title);
    return title;
}

void nav_title_close(NAV_TITLE *title)
{
    int ii;

    mpls_free(title->pl);
    for (ii = 0; ii < title->pl->list_count; ii++) {
        clpi_free(title->clip_list.clip[ii].cl);
    }
    X_FREE(title->clip_list.clip);
    X_FREE(title);
}

// Search for random access point closest to the requested packet
// Packets are 192 byte TS packets
NAV_CLIP* nav_chapter_search(NAV_TITLE *title, int chapter, uint32_t *out_pkt)
{
    if (chapter > title->chap_list.count) {
        *out_pkt = title->clip_list.clip[0].start_pkt;
        return &title->clip_list.clip[0];
    }
    *out_pkt = title->chap_list.chapter[chapter].clip_pkt;
    return &title->clip_list.clip[title->chap_list.chapter[chapter].clip_ref];
}

// Search for random access point closest to the requested packet
// Packets are 192 byte TS packets
// pkt is relative to the beginning of the title
// out_pkt and out_time is relative to the the clip which the packet falls in
NAV_CLIP* nav_packet_search(NAV_TITLE *title, uint32_t pkt, uint32_t *out_pkt, uint32_t *out_time)
{
    uint32_t pos, len;
    NAV_CLIP *clip;
    int ii;

    pos = 0;
    for (ii = 0; ii < title->pl->list_count; ii++) {
        clip = &title->clip_list.clip[ii];
        len = clip->end_pkt - clip->start_pkt;
        if (pkt < pos + len)
            break;
        pos += len;
    }
    if (ii == title->pl->list_count) {
        clip = &title->clip_list.clip[ii-1];
        *out_pkt = clip->end_pkt;
    } else {
        clip = &title->clip_list.clip[ii];
        *out_pkt = clpi_access_point(clip->cl, pkt - pos + clip->start_pkt, 0, 0, out_time);
    }
    return clip;
}

// Search for the nearest random access point after the given pkt
// which is an angle change point.
// Packets are 192 byte TS packets
// pkt is relative to the clip
// time is the clip relative time where the angle change point occurs
// returns a packet number
//
// To perform a seamless angle change, perform the following sequence:
// 1. Find the next angle change point with nav_angle_change_search.
// 2. Read and process packets until the angle change point is reached.
//    This may mean progressing to the next play item if the angle change
//    point is at the end of the current play item.
// 3. Change angles with nav_set_angle. Changing angles means changing
//    m2ts files. The new clip information is returned from nav_set_angle.
// 4. Close the current m2ts file and open the new one returned 
//    from nav_set_angle.
// 4. If the angle change point was within the time period of the current
//    play item (i.e. the angle change point is not at the end of the clip),
//    Search to the timestamp obtained from nav_angle_change using
//    nav_time_search. Otherwise start at the start_pkt defined by the clip.
uint32_t nav_angle_change_search(NAV_CLIP *clip, uint32_t pkt, uint32_t *time)
{
    return clpi_access_point(clip->cl, pkt, 1, 1, time);
}

// Search for random access point closest to the requested time
// Time is in 45khz ticks
NAV_CLIP* nav_time_search(NAV_TITLE *title, uint32_t tick, uint32_t *out_pkt)
{
    uint32_t pos, len;
    MPLS_PI *pi;
    NAV_CLIP *clip;
    int ii;

    pos = 0;
    for (ii = 0; ii < title->pl->list_count; ii++) {
        pi = &title->pl->play_item[ii];
        len = pi->out_time - pi->in_time;
        if (tick < pos + len)
            break;
        pos += len;
    }
    if (ii == title->pl->list_count) {
        clip = &title->clip_list.clip[ii-1];
        *out_pkt = clip->end_pkt;
    } else {
        clip = &title->clip_list.clip[ii];
        *out_pkt = clpi_lookup_spn(clip->cl, tick - pos + pi->in_time, 1,
                      title->pl->play_item[clip->ref].clip[clip->angle].stc_id);
    }
    return clip;
}

/*
 * Input Parameters:
 * title     - title struct obtained from nav_title_open
 *
 * Return value:
 * Pointer to NAV_CLIP struct
 * NULL - End of clip list
 */
NAV_CLIP* nav_next_clip(NAV_TITLE *title, NAV_CLIP *clip)
{
    if (clip == NULL) {
        return &title->clip_list.clip[0];
    }
    if (clip->ref >= title->clip_list.count - 1) {
        return NULL;
    }
    return &title->clip_list.clip[clip->ref + 1];
}

NAV_CLIP* nav_set_angle(NAV_TITLE *title, NAV_CLIP *clip, int angle)
{
    char *path;
    int ii;

    if (title == NULL) {
        return clip;
    }
    if (angle < 0 || angle > 8) {
        // invalid angle
        return clip;
    }
    title->angle = angle;
    // Find length in packets and end_pkt for each clip
    title->packets = 0;
    for (ii = 0; ii < title->pl->list_count; ii++) {
        MPLS_PI *pi;
        NAV_CLIP *clip;

        pi = &title->pl->play_item[ii];
        clip = &title->clip_list.clip[ii];
        if (title->angle >= pi->angle_count) {
            clip->angle = 0;
        } else {
            clip->angle = title->angle;
        }

        clpi_free(clip->cl);

        clip->ref = ii;
        strncpy(clip->name, pi->clip[clip->angle].clip_id, 5);
        strncpy(&clip->name[5], ".m2ts", 6);

        path = str_printf("%s"DIR_SEP"BDMV"DIR_SEP"CLIPINF"DIR_SEP"%s.clpi",
                      title->root, pi->clip[clip->angle].clip_id);
        clip->cl = clpi_parse(path, 0);
        X_FREE(path);
        if (clip->cl == NULL) {
            clip->start_pkt = 0;
            clip->end_pkt = 0;
            continue;
        }
        switch (pi->connection_condition) {
            case 5:
            case 6:
                clip->start_pkt = 0;
                clip->connection = CONNECT_SEAMLESS;
                break;
            default:
                clip->start_pkt = clpi_lookup_spn(clip->cl, pi->in_time, 1,
                                                  pi->clip[clip->angle].stc_id);
                clip->connection = CONNECT_NON_SEAMLESS;
            break;
        }
        clip->end_pkt = clpi_lookup_spn(clip->cl, pi->out_time, 0,
                                        pi->clip[clip->angle].stc_id);
    }
    _extrapolate(title);
    return clip;
}


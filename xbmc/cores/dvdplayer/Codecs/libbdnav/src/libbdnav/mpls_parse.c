#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include "../util/macro.h"
#include "../file/file.h"
#include "../util/bits.h"
#include "mpls_parse.h"

#define MPLS_SIG1  ('M' << 24 | 'P' << 16 | 'L' << 8 | 'S')
#define MPLS_SIG2A ('0' << 24 | '2' << 16 | '0' << 8 | '0')
#define MPLS_SIG2B ('0' << 24 | '1' << 16 | '0' << 8 | '0')

static int mpls_verbose = 0;

static void
_human_readable_sig(char *sig, uint32_t s1, uint32_t s2)
{
    sig[0] = (s1 >> 24) & 0xFF;
    sig[1] = (s1 >> 16) & 0xFF;
    sig[2] = (s1 >>  8) & 0xFF;
    sig[3] = (s1      ) & 0xFF;
    sig[4] = (s2 >> 24) & 0xFF;
    sig[5] = (s2 >> 16) & 0xFF;
    sig[6] = (s2 >>  8) & 0xFF;
    sig[7] = (s2      ) & 0xFF;
    sig[8] = 0;
}

static int
_parse_uo(BITSTREAM *bits, MPLS_UO *uo)
{
    uo->menu_call                       = bs_read(bits, 1);
    uo->title_search                    = bs_read(bits, 1);
    uo->chapter_search                  = bs_read(bits, 1);
    uo->time_search                     = bs_read(bits, 1);
    uo->skip_to_next_point              = bs_read(bits, 1);
    uo->skip_to_prev_point              = bs_read(bits, 1);
    uo->play_firstplay                  = bs_read(bits, 1);
    uo->stop                            = bs_read(bits, 1);
    uo->pause_on                        = bs_read(bits, 1);
    uo->pause_off                       = bs_read(bits, 1);
    uo->still                           = bs_read(bits, 1);
    uo->forward                         = bs_read(bits, 1);
    uo->backward                        = bs_read(bits, 1);
    uo->resume                          = bs_read(bits, 1);
    uo->move_up                         = bs_read(bits, 1);
    uo->move_down                       = bs_read(bits, 1);
    uo->move_left                       = bs_read(bits, 1);
    uo->move_right                      = bs_read(bits, 1);
    uo->select                          = bs_read(bits, 1);
    uo->activate                        = bs_read(bits, 1);
    uo->select_and_activate             = bs_read(bits, 1);
    uo->primary_audio_change            = bs_read(bits, 1);
    bs_skip(bits, 1);
    uo->angle_change                    = bs_read(bits, 1);
    uo->popup_on                        = bs_read(bits, 1);
    uo->popup_off                       = bs_read(bits, 1);
    uo->pg_enable_disable               = bs_read(bits, 1);
    uo->pg_change                       = bs_read(bits, 1);
    uo->secondary_video_enable_disable  = bs_read(bits, 1);
    uo->secondary_video_change          = bs_read(bits, 1);
    uo->secondary_audio_enable_disable  = bs_read(bits, 1);
    uo->secondary_audio_change          = bs_read(bits, 1);
    bs_skip(bits, 1);
    uo->pip_pg_change                   = bs_read(bits, 1);
    bs_skip(bits, 30);
    return 1;
}

static int
_parse_appinfo(BITSTREAM *bits, MPLS_AI *ai)
{
    int len;
    int pos;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_appinfo: alignment error\n");
    }
    pos = bs_pos(bits) >> 3;
    len = bs_read(bits, 32);

    // Reserved
    bs_skip(bits, 8);
    ai->playback_type = bs_read(bits, 8);
    if (ai->playback_type == 2 || ai->playback_type == 3) {
        ai->playback_count = bs_read(bits, 16);
    } else {
        // Reserved
        bs_skip(bits, 16);
    }
    _parse_uo(bits, &ai->uo_mask);
    ai->random_access_flag = bs_read(bits, 1);
    ai->audio_mix_flag = bs_read(bits, 1);
    ai->lossless_bypass_flag = bs_read(bits, 1);
    // Reserved
    bs_skip(bits, 13);
    bs_seek_byte(bits, pos + len);
    return 1;
}

static int
_parse_header(BITSTREAM *bits, MPLS_PL *pl)
{
    pl->type_indicator  = bs_read(bits, 32);
    pl->type_indicator2 = bs_read(bits, 32);
    if (pl->type_indicator != MPLS_SIG1 || 
        (pl->type_indicator2 != MPLS_SIG2A && 
         pl->type_indicator2 != MPLS_SIG2B)) {

        char sig[9];
        char expect[9];

        _human_readable_sig(sig, pl->type_indicator, pl->type_indicator2);
        _human_readable_sig(expect, MPLS_SIG1, MPLS_SIG2A);
        fprintf(stderr, "failed signature match, expected (%s) got (%s)\n", 
                expect, sig);
        return 0;
    }
    pl->list_pos = bs_read(bits, 32);
    pl->mark_pos = bs_read(bits, 32);
    pl->ext_pos  = bs_read(bits, 32);

    // Skip 160 reserved bits
    bs_skip(bits, 160);

    _parse_appinfo(bits, &pl->app_info);
    return 1;
}

static int
_parse_stream(BITSTREAM *bits, MPLS_STREAM *s)
{
    int len;
    int pos;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_stream: Stream alignment error\n");
    }
    len = bs_read(bits, 8);
    pos = bs_pos(bits) >> 3;

    s->stream_type = bs_read(bits, 8);
    switch (s->stream_type) {
        case 1:
            s->pid = bs_read(bits, 16);
            break;

        case 2:
        case 4:
            s->subpath_id = bs_read(bits, 8);
            s->subclip_id = bs_read(bits, 8);
            s->pid        = bs_read(bits, 16);
            break;

        case 3:
            s->subpath_id = bs_read(bits, 8);
            s->pid        = bs_read(bits, 16);
            break;

        default:
            fprintf(stderr, "unrecognized stream type %02x\n", s->stream_type);
            break;
    };

    bs_seek_byte(bits, pos + len);

    len = bs_read(bits, 8);
    pos = bs_pos(bits) >> 3;

    s->lang[0] = '\0';
    s->coding_type = bs_read(bits, 8);
    switch (s->coding_type) {
        case 0x01:
        case 0x02:
        case 0xea:
        case 0x1b:
            s->format = bs_read(bits, 4);
            s->rate   = bs_read(bits, 4);
            break;

        case 0x03:
        case 0x04:
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
            s->format = bs_read(bits, 4);
            s->rate   = bs_read(bits, 4);
            bs_read_bytes(bits, s->lang, 3);
            break;

        case 0x90:
        case 0x91:
            bs_read_bytes(bits, s->lang, 3);
            break;

        case 0x92:
            s->char_code = bs_read(bits, 8);
            bs_read_bytes(bits, s->lang, 3);
            break;

        default:
            fprintf(stderr, "unrecognized coding type %02x\n", s->coding_type);
            break;
    };
    s->lang[3] = '\0';

    bs_seek_byte(bits, pos + len);
    return 1;
}

static int
_parse_stn(BITSTREAM *bits, MPLS_STN *stn)
{
    int len;
    int pos;
    MPLS_STREAM *ss;
    int ii;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_stream: Stream alignment error\n");
    }
    // Skip STN len
    len = bs_read(bits, 16);
    pos = bs_pos(bits) >> 3;

    // Skip 2 reserved bytes
    bs_skip(bits, 16);

    stn->num_video           = bs_read(bits, 8);
    stn->num_audio           = bs_read(bits, 8);
    stn->num_pg              = bs_read(bits, 8);
    stn->num_ig              = bs_read(bits, 8);
    stn->num_secondary_audio = bs_read(bits, 8);
    stn->num_secondary_video = bs_read(bits, 8);
    stn->num_pip_pg          = bs_read(bits, 8);

    // 5 reserve bytes
    bs_skip(bits, 5 * 8);

    ss = NULL;
    if (stn->num_video) {
        ss = calloc(stn->num_video, sizeof(MPLS_STREAM));
    }
    for (ii = 0; ii < stn->num_video; ii++) {
        if (!_parse_stream(bits, &ss[ii])) {
            X_FREE(ss);
            fprintf(stderr, "error parsing video entry\n");
            return 0;
        }
    }
    stn->video = ss;

    ss = NULL;
    if (stn->num_audio)
        ss = calloc(stn->num_audio, sizeof(MPLS_STREAM));
    for (ii = 0; ii < stn->num_audio; ii++) {

        if (!_parse_stream(bits, &ss[ii])) {
            X_FREE(ss);
            fprintf(stderr, "error parsing audio entry\n");
            return 0;
        }
    }
    stn->audio = ss;

    ss = NULL;
    if (stn->num_pg) {
        ss = calloc(stn->num_pg, sizeof(MPLS_STREAM));
    }
    for (ii = 0; ii < stn->num_pg; ii++) {
        if (!_parse_stream(bits, &ss[ii])) {
            X_FREE(ss);
            fprintf(stderr, "error parsing pg entry\n");
            return 0;
        }
    }
    stn->pg = ss;

    bs_seek_byte(bits, pos + len);
    return 1;
}

static void
_clean_stn(MPLS_STN *stn)
{
    X_FREE(stn->video);
    X_FREE(stn->audio);
    X_FREE(stn->pg);
}

static int
_parse_playitem(BITSTREAM *bits, MPLS_PI *pi)
{
    int len, ii;
    int pos;
    char clip_id[6], codec_id[5];
    uint8_t stc_id;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_playitem: Stream alignment error\n");
    }

    // PlayItem Length
    len = bs_read(bits, 16);
    pos = bs_pos(bits) >> 3;

    // Primary Clip identifer
    bs_read_bytes(bits, (uint8_t*)clip_id, 5);
    clip_id[5] = '\0';

    bs_read_bytes(bits, (uint8_t*)codec_id, 4);
    codec_id[4] = '\0';
    if (memcmp(codec_id, "M2TS", 4) != 0) {
        fprintf(stderr, "Incorrect CodecIdentifier (%s)\n", codec_id);
    }

    // Skip reserved 11 bits
    bs_skip(bits, 11);

    pi->is_multi_angle = bs_read(bits, 1);

    pi->connection_condition = bs_read(bits, 4);
    if (pi->connection_condition != 0x01 && 
        pi->connection_condition != 0x05 &&
        pi->connection_condition != 0x06) {

        fprintf(stderr, "Unexpected connection condition %02x\n", 
                pi->connection_condition);
    }

    stc_id   = bs_read(bits, 8);
    pi->in_time  = bs_read(bits, 32);
    pi->out_time = bs_read(bits, 32);

    _parse_uo(bits, &pi->uo_mask);
    pi->random_access_flag = bs_read(bits, 1);
    bs_skip(bits, 7);
    pi->still_mode = bs_read(bits, 8);
    if (pi->still_mode == 0x01) {
        pi->still_time = bs_read(bits, 16);
    } else {
        bs_skip(bits, 16);
    }

    pi->angle_count = 1;
    if (pi->is_multi_angle) {
        pi->angle_count = bs_read(bits, 8);
        if (pi->angle_count < 1) {
            pi->angle_count = 1;
        }
        bs_skip(bits, 6);
        pi->is_different_audio = bs_read(bits, 1);
        pi->is_seamless_angle = bs_read(bits, 1);
    }
    pi->clip = calloc(pi->angle_count, sizeof(MPLS_CLIP));
    strcpy(pi->clip[0].clip_id, clip_id);
    strcpy(pi->clip[0].codec_id, codec_id);
    pi->clip[0].stc_id = stc_id;
    for (ii = 1; ii < pi->angle_count; ii++) {
        bs_read_bytes(bits, (uint8_t*)pi->clip[ii].clip_id, 5);
        pi->clip[ii].clip_id[5] = '\0';

        bs_read_bytes(bits, (uint8_t*)pi->clip[ii].codec_id, 4);
        pi->clip[ii].codec_id[4] = '\0';
        if (memcmp(pi->clip[ii].codec_id, "M2TS", 4) != 0) {
            fprintf(stderr, "Incorrect CodecIdentifier (%s)\n", pi->clip[ii].codec_id);
        }
        pi->clip[ii].stc_id   = bs_read(bits, 8);
    }
    if (!_parse_stn(bits, &pi->stn)) {
        return 0;
    }
    // Seek past any unused items
    bs_seek_byte(bits, pos + len);
    return 1;
}

static void
_clean_playitem(MPLS_PI *pi)
{
    X_FREE(pi->clip);
    _clean_stn(&pi->stn);
}

static int
_parse_subplayitem(BITSTREAM *bits, MPLS_SUB_PI *spi)
{
    int len, ii;
    int pos;
    char clip_id[6], codec_id[5];
    uint8_t stc_id;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_subplayitem: alignment error\n");
    }

    // PlayItem Length
    len = bs_read(bits, 16);
    pos = bs_pos(bits) >> 3;

    // Primary Clip identifer
    bs_read_bytes(bits, (uint8_t*)clip_id, 5);
    clip_id[5] = '\0';

    bs_read_bytes(bits, (uint8_t*)codec_id, 4);
    codec_id[4] = '\0';
    if (memcmp(codec_id, "M2TS", 4) != 0) {
        fprintf(stderr, "Incorrect CodecIdentifier (%s)\n", codec_id);
    }

    bs_skip(bits, 27);

    spi->connection_condition = bs_read(bits, 4);

    if (spi->connection_condition != 0x01 && 
        spi->connection_condition != 0x05 &&
        spi->connection_condition != 0x06) {

        fprintf(stderr, "Unexpected connection condition %02x\n", 
                spi->connection_condition);
    }
    spi->is_multi_clip     = bs_read(bits, 1);
    stc_id                 = bs_read(bits, 8);
    spi->in_time           = bs_read(bits, 32);
    spi->out_time          = bs_read(bits, 32);
    spi->sync_play_item_id = bs_read(bits, 16);
    spi->sync_pts          = bs_read(bits, 32);
    spi->clip_count = 1;
    if (spi->is_multi_clip) {
        spi->clip_count    = bs_read(bits, 8);
        if (spi->clip_count < 1) {
            spi->clip_count = 1;
        }
    }
    spi->clip = calloc(spi->clip_count, sizeof(MPLS_CLIP));
    strcpy(spi->clip[0].clip_id, clip_id);
    strcpy(spi->clip[0].codec_id, codec_id);
    spi->clip[0].stc_id = stc_id;
    for (ii = 1; ii < spi->clip_count; ii++) {
        // Primary Clip identifer
        bs_read_bytes(bits, (uint8_t*)spi->clip[ii].clip_id, 5);
        spi->clip[ii].clip_id[5] = '\0';

        bs_read_bytes(bits, (uint8_t*)spi->clip[ii].codec_id, 4);
        spi->clip[ii].codec_id[4] = '\0';
        if (memcmp(spi->clip[ii].codec_id, "M2TS", 4) != 0) {
            fprintf(stderr, "Incorrect CodecIdentifier (%s)\n", spi->clip[ii].codec_id);
        }
        spi->clip[ii].stc_id = bs_read(bits, 8);
    }


    // Seek to end of subpath
    bs_seek_byte(bits, pos + len);
    return 1;
}

static void
_clean_subplayitem(MPLS_SUB_PI *spi)
{
    X_FREE(spi->clip);
}

static int
_parse_subpath(BITSTREAM *bits, MPLS_SUB *sp)
{
    int len, ii;
    int pos;
    MPLS_SUB_PI *spi = NULL;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_subpath: alignment error\n");
    }

    // PlayItem Length
    len = bs_read(bits, 32);
    pos = bs_pos(bits) >> 3;

    bs_skip(bits, 8);
    sp->type = bs_read(bits, 8);
    bs_skip(bits, 15);
    sp->is_repeat = bs_read(bits, 1);
    bs_skip(bits, 8);
    sp->sub_playitem_count = bs_read(bits, 8);

    spi = calloc(sp->sub_playitem_count,  sizeof(MPLS_SUB_PI));
    for (ii = 0; ii < sp->sub_playitem_count; ii++) {
        if (!_parse_subplayitem(bits, &spi[ii])) {
            X_FREE(spi);
            fprintf(stderr, "error parsing sub play item\n");
            return 0;
        }
    }
    sp->sub_play_item = spi;

    // Seek to end of subpath
    bs_seek_byte(bits, pos + len);
    return 1;
}

static void
_clean_subpath(MPLS_SUB *sp)
{
    int ii;

    for (ii = 0; ii < sp->sub_playitem_count; ii++) {
        _clean_subplayitem(&sp->sub_play_item[ii]);
    }
    X_FREE(sp->sub_play_item);
}

static int
_parse_playlistmark(BITSTREAM *bits, MPLS_PL *pl)
{
    int ii;
    MPLS_PLM *plm = NULL;

    bs_seek_byte(bits, pl->mark_pos);
    // Skip the length field, I don't use it
    bs_skip(bits, 32);
    // Then get the number of marks
    pl->mark_count = bs_read(bits, 16);

    plm = calloc(pl->mark_count, sizeof(MPLS_PLM));
    for (ii = 0; ii < pl->mark_count; ii++) {
        plm[ii].mark_id       = bs_read(bits, 8);
        plm[ii].mark_type     = bs_read(bits, 8);
        plm[ii].play_item_ref = bs_read(bits, 16);
        plm[ii].time          = bs_read(bits, 32);
        plm[ii].entry_es_pid  = bs_read(bits, 16);
        plm[ii].duration      = bs_read(bits, 32);
    }
    pl->play_mark = plm;
    return 1;
}

static int
_parse_playlist(BITSTREAM *bits, MPLS_PL *pl)
{
    int ii;
    MPLS_PI *pi = NULL;
    MPLS_SUB *sub_path = NULL;

    bs_seek_byte(bits, pl->list_pos);
    // Skip playlist length
    bs_skip(bits, 32);
    // Skip reserved bytes
    bs_skip(bits, 16);

    pl->list_count = bs_read(bits, 16);
    pl->sub_count = bs_read(bits, 16);

    pi = calloc(pl->list_count,  sizeof(MPLS_PI));
    for (ii = 0; ii < pl->list_count; ii++) {
        if (!_parse_playitem(bits, &pi[ii])) {
            X_FREE(pi);
            fprintf(stderr, "error parsing play list item\n");
            return 0;
        }
    }
    pl->play_item = pi;

    sub_path = calloc(pl->sub_count,  sizeof(MPLS_SUB));
    for (ii = 0; ii < pl->sub_count; ii++)
    {
        if (!_parse_subpath(bits, &sub_path[ii]))
        {
            X_FREE(sub_path);
            fprintf(stderr, "error parsing subpath\n");
            return 0;
        }
    }
    pl->sub_path = sub_path;

    return 1;
}

static void
_clean_playlist(MPLS_PL *pl)
{
    int ii;

    if (pl == NULL) {
        return;
    }
    if (pl->play_item != NULL) {
        for (ii = 0; ii < pl->list_count; ii++) {
            _clean_playitem(&pl->play_item[ii]);
        }
        X_FREE(pl->play_item);
    }
    if (pl->sub_path != NULL) {
        for (ii = 0; ii < pl->sub_count; ii++) {
            _clean_subpath(&pl->sub_path[ii]);
        }
        X_FREE(pl->sub_path);
    }
    X_FREE(pl->play_mark);
    X_FREE(pl);
}

void
mpls_free(MPLS_PL *pl)
{
    _clean_playlist(pl);
}

MPLS_PL*
mpls_parse(char *path, int verbose)
{
    BITSTREAM  bits;
    FILE_H    *fp;
    MPLS_PL   *pl = NULL;

    mpls_verbose = verbose;

    pl = calloc(1, sizeof(MPLS_PL));
    if (pl == NULL) {
        return NULL;
    }

    fp = file_open(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        X_FREE(pl);
        return NULL;
    }

    bs_init(&bits, fp);
    if (!_parse_header(&bits, pl)) {
        file_close(fp);
        _clean_playlist(pl);
        return NULL;
    }
    if (!_parse_playlist(&bits, pl)) {
        file_close(fp);
        _clean_playlist(pl);
        return NULL;
    }
    if (!_parse_playlistmark(&bits, pl)) {
        file_close(fp);
        _clean_playlist(pl);
        return NULL;
    }
    file_close(fp);
    return pl;
}


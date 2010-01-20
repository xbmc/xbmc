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
#include "clpi_parse.h"

#define CLPI_SIG1  ('H' << 24 | 'D' << 16 | 'M' << 8 | 'V')
#define CLPI_SIG2A ('0' << 24 | '2' << 16 | '0' << 8 | '0')
#define CLPI_SIG2B ('0' << 24 | '1' << 16 | '0' << 8 | '0')

static int clpi_verbose = 0;

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
_parse_stream_attr(BITSTREAM *bits, CLPI_PROG_STREAM *ss)
{
    int len, pos;

    if (!bs_is_align(bits, 0x07)) {
        fprintf(stderr, "_parse_stream_attr: Stream alignment error\n");
    }

    len = bs_read(bits, 8);
    pos = bs_pos(bits) >> 3;

    ss->lang[0] = '\0';
    ss->coding_type = bs_read(bits, 8);
    switch (ss->coding_type) {
        case 0x01:
        case 0x02:
        case 0xea:
        case 0x1b:
            ss->format = bs_read(bits, 4);
            ss->rate   = bs_read(bits, 4);
            ss->aspect = bs_read(bits, 4);
            bs_skip(bits, 2);
            ss->oc_flag = bs_read(bits, 1);
            bs_skip(bits, 1);
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
            ss->format = bs_read(bits, 4);
            ss->rate   = bs_read(bits, 4);
            bs_read_bytes(bits, ss->lang, 3);
            break;

        case 0x90:
        case 0x91:
        case 0xa0:
            bs_read_bytes(bits, ss->lang, 3);
            break;

        case 0x92:
            ss->char_code = bs_read(bits, 8);
            bs_read_bytes(bits, ss->lang, 3);
            break;

        default:
            fprintf(stderr, "stream attr: unrecognized coding type %02x\n", ss->coding_type);
            break;
    };
    ss->lang[3] = '\0';

    // Skip over any padding
    bs_seek_byte(bits, pos + len);
    return 1;
}

static int
_parse_header(BITSTREAM *bits, CLPI_CL *cl)
{
    bs_seek_byte(bits, 0);
    cl->type_indicator  = bs_read(bits, 32);
    cl->type_indicator2 = bs_read(bits, 32);
    if (cl->type_indicator != CLPI_SIG1 || 
        (cl->type_indicator2 != CLPI_SIG2A &&
         cl->type_indicator2 != CLPI_SIG2B)) {

        char sig[9];
        char expect[9];

        _human_readable_sig(sig, cl->type_indicator, cl->type_indicator2);
        _human_readable_sig(expect, CLPI_SIG1, CLPI_SIG2A);
        fprintf(stderr, "failed signature match expected (%s) got (%s)\n", 
                expect, sig);
        return 0;
    }
    cl->sequence_info_start_addr = bs_read(bits, 32);
    cl->program_info_start_addr = bs_read(bits, 32);
    cl->cpi_start_addr = bs_read(bits, 32);
    cl->clip_mark_start_addr = bs_read(bits, 32);
    cl->ext_data_start_addr = bs_read(bits, 32);
    return 1;
}

static int
_parse_clipinfo(BITSTREAM *bits, CLPI_CL *cl)
{
    int len, pos;
    int ii;

    bs_seek_byte(bits, 40);
    // ClipInfo len
    bs_skip(bits, 32);
    // reserved
    bs_skip(bits, 16);
    cl->clip.clip_stream_type = bs_read(bits, 8);
    cl->clip.application_type = bs_read(bits, 8);
    // skip reserved 31 bits
    bs_skip(bits, 31);
    cl->clip.is_atc_delta       = bs_read(bits, 1);
    cl->clip.ts_recording_rate  = bs_read(bits, 32);
    cl->clip.num_source_packets = bs_read(bits, 32);

    // Skip reserved 128 bytes
    bs_skip(bits, 128 * 8);

    // ts type info block
    len = bs_read(bits, 16);
    pos = bs_pos(bits) >> 3;
    if (len) {
        cl->clip.ts_type_info.validity = bs_read(bits, 8);
        bs_read_bytes(bits, cl->clip.ts_type_info.format_id, 4);
        cl->clip.ts_type_info.format_id[4] = '\0';
        // Seek past the stuff we don't know anything about
        bs_seek_byte(bits, pos + len);
    }
    if (cl->clip.is_atc_delta) {
        // Skip reserved bytes
        bs_skip(bits, 8);
        cl->clip.atc_delta_count = bs_read(bits, 8);
        cl->clip.atc_delta = 
            malloc(cl->clip.atc_delta_count * sizeof(CLPI_ATC_DELTA));
        for (ii = 0; ii < cl->clip.atc_delta_count; ii++) {
            cl->clip.atc_delta[ii].delta = bs_read(bits, 32);
            bs_read_bytes(bits, cl->clip.atc_delta[ii].file_id, 5);
            cl->clip.atc_delta[ii].file_id[5] = '\0';
            bs_read_bytes(bits, cl->clip.atc_delta[ii].file_code, 4);
            cl->clip.atc_delta[ii].file_code[4] = '\0';
            bs_skip(bits, 8);
        }
    }
    return 1;
}

static int
_parse_sequence(BITSTREAM *bits, CLPI_CL *cl)
{
    int ii, jj;

    bs_seek_byte(bits, cl->sequence_info_start_addr);

    // Skip the length field, and a reserved byte
    bs_skip(bits, 5 * 8);
    // Then get the number of sequences
    cl->sequence.num_atc_seq = bs_read(bits, 8);

    CLPI_ATC_SEQ *atc_seq;
    atc_seq = malloc(cl->sequence.num_atc_seq * sizeof(CLPI_ATC_SEQ));
    cl->sequence.atc_seq = atc_seq;
    for (ii = 0; ii < cl->sequence.num_atc_seq; ii++) {
        atc_seq[ii].spn_atc_start = bs_read(bits, 32);
        atc_seq[ii].num_stc_seq   = bs_read(bits, 8);
        atc_seq[ii].offset_stc_id = bs_read(bits, 8);

        CLPI_STC_SEQ *stc_seq;
        stc_seq = malloc(atc_seq[ii].num_stc_seq * sizeof(CLPI_STC_SEQ));
        atc_seq[ii].stc_seq = stc_seq;
        for (jj = 0; jj < atc_seq[ii].num_stc_seq; jj++) {
            stc_seq[jj].pcr_pid                 = bs_read(bits, 16);
            stc_seq[jj].spn_stc_start           = bs_read(bits, 32);
            stc_seq[jj].presentation_start_time = bs_read(bits, 32);
            stc_seq[jj].presentation_end_time   = bs_read(bits, 32);
        }
    }
    return 1;
}

static int
_parse_program(BITSTREAM *bits, CLPI_CL *cl)
{
    int ii, jj;

    bs_seek_byte(bits, cl->program_info_start_addr);
    // Skip the length field, and a reserved byte
    bs_skip(bits, 5 * 8);
    // Then get the number of sequences
    cl->program.num_prog = bs_read(bits, 8);

    CLPI_PROG *progs;
    progs = malloc(cl->program.num_prog * sizeof(CLPI_PROG));
    cl->program.progs = progs;
    for (ii = 0; ii < cl->program.num_prog; ii++) {
        progs[ii].spn_program_sequence_start = bs_read(bits, 32);
        progs[ii].program_map_pid            = bs_read(bits, 16);
        progs[ii].num_streams                = bs_read(bits, 8);
        progs[ii].num_groups                 = bs_read(bits, 8);

        CLPI_PROG_STREAM *ps;
        ps = malloc(progs[ii].num_streams * sizeof(CLPI_PROG_STREAM));
        progs[ii].streams = ps;
        for (jj = 0; jj < progs[ii].num_streams; jj++) {
            ps[jj].pid = bs_read(bits, 16);
            if (!_parse_stream_attr(bits, &ps[jj])) {
                return 0;
            }
        }
    }
    return 1;
}

static int
_parse_ep_map_stream(BITSTREAM *bits, CLPI_EP_MAP_ENTRY *ee)
{
    uint32_t          fine_start;
    int               ii;
    CLPI_EP_COARSE   * coarse;
    CLPI_EP_FINE     * fine;

    bs_seek_byte(bits, ee->ep_map_stream_start_addr);
    fine_start = bs_read(bits, 32);

    coarse = malloc(ee->num_ep_coarse * sizeof(CLPI_EP_COARSE));
    ee->coarse = coarse;
    for (ii = 0; ii < ee->num_ep_coarse; ii++) {
        coarse[ii].ref_ep_fine_id = bs_read(bits, 18);
        coarse[ii].pts_ep         = bs_read(bits, 14);
        coarse[ii].spn_ep         = bs_read(bits, 32);
    }

    bs_seek_byte(bits, ee->ep_map_stream_start_addr+fine_start);

    fine = malloc(ee->num_ep_fine * sizeof(CLPI_EP_COARSE));
    ee->fine = fine;
    for (ii = 0; ii < ee->num_ep_fine; ii++) {
        fine[ii].is_angle_change_point = bs_read(bits, 1);
        fine[ii].i_end_position_offset = bs_read(bits, 3);
        fine[ii].pts_ep                =  bs_read(bits, 11);
        fine[ii].spn_ep                =  bs_read(bits, 17);
    }
    return 1;
}

static int
_parse_cpi(BITSTREAM *bits, CLPI_CL *cl)
{
    int ii;
    uint32_t ep_map_pos, len;

    bs_seek_byte(bits, cl->cpi_start_addr);
    len = bs_read(bits, 32);
    if (len == 0) {
        return 1;
    }

    bs_skip(bits, 12);
    cl->cpi.type = bs_read(bits, 4);
    ep_map_pos = bs_pos(bits) >> 3;

    // EP Map starts here
    bs_skip(bits, 8);
    cl->cpi.num_stream_pid = bs_read(bits, 8);

    CLPI_EP_MAP_ENTRY *entry;
    entry = malloc(cl->cpi.num_stream_pid * sizeof(CLPI_EP_MAP_ENTRY));
    cl->cpi.entry = entry;
    for (ii = 0; ii < cl->cpi.num_stream_pid; ii++) {
        entry[ii].pid                      = bs_read(bits, 16);
        bs_skip(bits, 10);
        entry[ii].ep_stream_type           = bs_read(bits, 4);
        entry[ii].num_ep_coarse            = bs_read(bits, 16);
        entry[ii].num_ep_fine              = bs_read(bits, 18);
        entry[ii].ep_map_stream_start_addr = bs_read(bits, 32) + ep_map_pos;
    }
    for (ii = 0; ii < cl->cpi.num_stream_pid; ii++) {
        _parse_ep_map_stream(bits, &cl->cpi.entry[ii]);
    }
    return 1;
}

uint32_t
_find_stc_spn(CLPI_CL *cl, uint8_t stc_id)
{
    int ii;
    CLPI_ATC_SEQ *atc;

    for (ii = 0; ii < cl->sequence.num_atc_seq; ii++) {
        atc = &cl->sequence.atc_seq[ii];
        if (stc_id < atc->offset_stc_id + atc->num_stc_seq) {
            return atc->stc_seq[stc_id - atc->offset_stc_id].spn_stc_start;
        }
    }
    return 0;
}

// Looks up the start packet number for the timestamp
// Returns the spn for the entry that is closest to but
// before the given timestamp
uint32_t
clpi_lookup_spn(CLPI_CL *cl, uint32_t timestamp, int before, uint8_t stc_id)
{
    CLPI_EP_MAP_ENTRY *entry;
    CLPI_CPI *cpi = &cl->cpi;
    int ii, jj;
    uint32_t coarse_pts, pts; // 45khz timestamps
    uint32_t spn, coarse_spn, stc_spn;
    int start, end;
    int ref;

    // Assumes that there is only one pid of interest
    entry = &cpi->entry[0];

    // Use sequence info to find spn_stc_start before doing
    // PTS search. The spn_stc_start defines the point in
    // the EP map to start searching.
    stc_spn = _find_stc_spn(cl, stc_id);
    for (ii = 0; ii < entry->num_ep_coarse; ii++) {
        ref = entry->coarse[ii].ref_ep_fine_id;
        if (entry->coarse[ii].spn_ep >= stc_spn) {
            // The desired starting point is either after this point
            // or in the middle of the previous coarse entry
            break;
        }
    }
    if (ii >= entry->num_ep_coarse) {
        return cl->clip.num_source_packets;
    }
    pts = ((uint64_t)(entry->coarse[ii].pts_ep & ~0x01) << 18) +
          ((uint64_t)entry->fine[ref].pts_ep << 8);
    if (pts > timestamp) {
        // The starting point and desired PTS is in the previous coarse entry
        ii--;
        coarse_pts = (uint32_t)(entry->coarse[ii].pts_ep & ~0x01) << 18;
        coarse_spn = entry->coarse[ii].spn_ep;
        start = entry->coarse[ii].ref_ep_fine_id;
        end = entry->coarse[ii+1].ref_ep_fine_id;
        // Find a fine entry that has bothe spn > stc_spn and ptc > timestamp
        for (jj = start; jj < end; jj++) {

            pts = coarse_pts + ((uint32_t)entry->fine[jj].pts_ep << 8);
            spn = (coarse_spn & ~0x1FFFF) + entry->fine[jj].spn_ep;
            if (stc_spn >= spn && pts > timestamp)
                break;
        }
        goto done;
    }

    // If we've gotten this far, the desired timestamp is somewhere
    // after the coarse entry we found the stc_spn in.
    start = ii;
    for (ii = start; ii < entry->num_ep_coarse; ii++) {
        ref = entry->coarse[ii].ref_ep_fine_id;
        pts = ((uint64_t)(entry->coarse[ii].pts_ep & ~0x01) << 18) +
                ((uint64_t)entry->fine[ref].pts_ep << 8);
        if (pts > timestamp) {
            break;
        }
    }
    // If the timestamp is before the first entry, then return
    // the beginning of the clip
    if (ii == 0) {
        return 0;
    }
    ii--;
    coarse_pts = (uint32_t)(entry->coarse[ii].pts_ep & ~0x01) << 18;
    start = entry->coarse[ii].ref_ep_fine_id;
    if (ii < entry->num_ep_coarse - 1) {
        end = entry->coarse[ii+1].ref_ep_fine_id;
    } else {
        end = entry->num_ep_fine;
    }
    for (jj = start; jj < end; jj++) {

        pts = coarse_pts + ((uint32_t)entry->fine[jj].pts_ep << 8);
        if (pts > timestamp)
            break;
    }

done:
    if (before) {
        jj--;
    }
    if (jj == end) {
        ii++;
        if (ii >= entry->num_ep_coarse) {
            // End of file
            return cl->clip.num_source_packets;
        }
        jj = entry->coarse[ii].ref_ep_fine_id;
    }
    spn = (entry->coarse[ii].spn_ep & ~0x1FFFF) + entry->fine[jj].spn_ep;
    return spn;
}

// Looks up the start packet number that is closest to the requested packet
// Returns the spn for the entry that is closest to but
// before the given packet
uint32_t
clpi_access_point(CLPI_CL *cl, uint32_t pkt, int next, int angle_change, uint32_t *time)
{
    CLPI_EP_MAP_ENTRY *entry;
    CLPI_CPI *cpi = &cl->cpi;
    int ii, jj;
    uint32_t coarse_spn, spn;
    int start, end;
    int ref;

    // Assumes that there is only one pid of interest
    entry = &cpi->entry[0];

    for (ii = 0; ii < entry->num_ep_coarse; ii++) {
        ref = entry->coarse[ii].ref_ep_fine_id;
        spn = (entry->coarse[ii].spn_ep & ~0x1FFFF) + entry->fine[ref].spn_ep;
        if (spn > pkt) {
            break;
        }
    }
    // If the timestamp is before the first entry, then return
    // the beginning of the clip
    if (ii == 0) {
        return 0;
    }
    ii--;
    coarse_spn = (entry->coarse[ii].spn_ep & ~0x1FFFF);
    start = entry->coarse[ii].ref_ep_fine_id;
    if (ii < entry->num_ep_coarse - 1) {
        end = entry->coarse[ii+1].ref_ep_fine_id;
    } else {
        end = entry->num_ep_fine;
    }
    for (jj = start; jj < end; jj++) {

        spn = coarse_spn + entry->fine[jj].spn_ep;
        if (spn >= pkt) {
            break;
        }
    }
    if (jj == end && next) {
        ii++;
        jj = 0;
    } else if (spn != pkt && !next) {
        jj--;
    }
    if (ii == entry->num_ep_coarse) {
        *time = 0;
        return cl->clip.num_source_packets;
    }
    coarse_spn = (entry->coarse[ii].spn_ep & ~0x1FFFF);
    if (angle_change) {
        // Keep looking till there's an angle change point
        for (; jj < end; jj++) {

            if (entry->fine[jj].is_angle_change_point) {
                *time = ((uint64_t)(entry->coarse[ii].pts_ep & ~0x01) << 18) +
                        ((uint64_t)entry->fine[jj].pts_ep << 8);
                return coarse_spn + entry->fine[jj].spn_ep;
            }
        }
        for (ii++; ii < entry->num_ep_coarse; ii++) {
            start = entry->coarse[ii].ref_ep_fine_id;
            if (ii < entry->num_ep_coarse - 1) {
                end = entry->coarse[ii+1].ref_ep_fine_id;
            } else {
                end = entry->num_ep_fine;
            }
            for (jj = start; jj < end; jj++) {

                if (entry->fine[jj].is_angle_change_point) {
                    *time = ((uint64_t)(entry->coarse[ii].pts_ep & ~0x01) << 18) +
                            ((uint64_t)entry->fine[jj].pts_ep << 8);
                    return coarse_spn + entry->fine[jj].spn_ep;
                }
            }
        }
        *time = 0;
        return cl->clip.num_source_packets;
    }
    return coarse_spn + entry->fine[jj].spn_ep;
}

void
clpi_free(CLPI_CL *cl)
{
    int ii;

    if (cl == NULL) {
        return;
    }
    if (cl->clip.atc_delta != NULL) {
        X_FREE(cl->clip.atc_delta);
    }
    for (ii = 0; ii < cl->sequence.num_atc_seq; ii++) {
        if (cl->sequence.atc_seq[ii].stc_seq != NULL) {
            X_FREE(cl->sequence.atc_seq[ii].stc_seq);
        }
    }
    if (cl->sequence.atc_seq != NULL) {
        X_FREE(cl->sequence.atc_seq);
    }

    for (ii = 0; ii < cl->program.num_prog; ii++) {
        if (cl->program.progs[ii].streams != NULL) {
            X_FREE(cl->program.progs[ii].streams);
        }
    }
    if (cl->program.progs != NULL) {
        X_FREE(cl->program.progs);
    }

    for (ii = 0; ii < cl->cpi.num_stream_pid; ii++) {
        if (cl->cpi.entry[ii].coarse != NULL) {
            X_FREE(cl->cpi.entry[ii].coarse);
        }
        if (cl->cpi.entry[ii].fine != NULL) {
            X_FREE(cl->cpi.entry[ii].fine);
        }
    }
    if (cl->cpi.entry != NULL) {
        X_FREE(cl->cpi.entry);
    }
    X_FREE(cl);
}

CLPI_CL*
clpi_parse(char *path, int verbose)
{
    BITSTREAM  bits;
    FILE_H    *fp;
    CLPI_CL   *cl;

    clpi_verbose = verbose;

    cl = calloc(1, sizeof(CLPI_CL));
    if (cl == NULL) {
        return NULL;
    }

    fp = file_open(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        X_FREE(cl);
        return NULL;
    }

    bs_init(&bits, fp);
    if (!_parse_header(&bits, cl)) {
        file_close(fp);
        clpi_free(cl);
        return NULL;
    }
    if (!_parse_clipinfo(&bits, cl)) {
        file_close(fp);
        clpi_free(cl);
        return NULL;
    }
    if (!_parse_sequence(&bits, cl)) {
        file_close(fp);
        clpi_free(cl);
        return NULL;
    }
    if (!_parse_program(&bits, cl)) {
        file_close(fp);
        clpi_free(cl);
        return NULL;
    }
    if (!_parse_cpi(&bits, cl)) {
        file_close(fp);
        clpi_free(cl);
        return NULL;
    }
    file_close(fp);
    return cl;
}


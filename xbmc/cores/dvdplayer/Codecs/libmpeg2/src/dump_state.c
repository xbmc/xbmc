/*
 * dump_state.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "mpeg2.h"

void dump_state (FILE * f, mpeg2_state_t state, const mpeg2_info_t * info,
		 int offset, int verbose);

static struct {
    const mpeg2_sequence_t * ptr;
    mpeg2_sequence_t value;
} last_sequence;
static struct {
    const mpeg2_gop_t * ptr;
    mpeg2_gop_t value;
} last_gop;
static struct save_buf {
    const mpeg2_fbuf_t * ptr;
    mpeg2_fbuf_t value;
} last_curbuf, last_dispbuf, last_discbuf, buf_code_list[26];
static int buf_code_index = 0;
static int buf_code_new = -1;
static struct save_pic {
    const mpeg2_picture_t * ptr;
    mpeg2_picture_t value;
} last_curpic, last_curpic2, last_disppic, last_disppic2, pic_code_list[26];
static int pic_code_index = 0;
static int pic_code_new = -1;

static int sequence_match (const mpeg2_sequence_t * seq)
{
    return (last_sequence.ptr == seq &&
	    (seq == NULL ||
	     !memcmp (seq, &last_sequence.value, sizeof (mpeg2_sequence_t))));
}

static void sequence_save (const mpeg2_sequence_t * seq)
{
    last_sequence.ptr = seq;
    if (seq != NULL)
	last_sequence.value = *seq;
}

static char seq_code (const mpeg2_sequence_t * seq)
{
    if (seq == NULL)
	return '-';
    else if (sequence_match (seq))
	return 'S';
    else
	return '?';
}

static int gop_match (const mpeg2_gop_t * gop)
{
    return (last_gop.ptr == gop &&
	    (gop == NULL ||
	     !memcmp (gop, &last_gop.value, sizeof (mpeg2_gop_t))));
}

static void gop_save (const mpeg2_gop_t * gop)
{
    last_gop.ptr = gop;
    if (gop != NULL)
	last_gop.value = *gop;
}

static char gop_code (const mpeg2_gop_t * gop)
{
    if (gop == NULL)
	return '-';
    else if (gop_match (gop))
	return 'G';
    else
	return '?';
}

static int fbuf_match (const mpeg2_fbuf_t * fbuf, struct save_buf * saved)
{
    return (saved->ptr == fbuf &&
	    (fbuf == NULL ||
	     !memcmp (fbuf, &saved->value, sizeof (mpeg2_fbuf_t))));
}

static void fbuf_save (const mpeg2_fbuf_t * fbuf, struct save_buf * saved)
{
    saved->ptr = fbuf;
    if (fbuf != NULL)
	saved->value = *fbuf;
}

static char buf_code (const mpeg2_fbuf_t * fbuf)
{
    int i;

    if (fbuf == NULL)
	return '-';
    for (i = 0; i < 26; i++)
	if (fbuf_match (fbuf, buf_code_list + i))
	    return ((i == buf_code_new) ? 'A' : 'a') + i;
    return '?';
}

static void buf_code_add (const mpeg2_fbuf_t * fbuf, FILE * f)
{
    int i;

    if (fbuf == NULL)
	fprintf (f, "buf_code_add error\n");
    for (i = 0; i < 26; i++)
	if (buf_code_list[i].ptr == fbuf)
	    fprintf (f, "buf_code_add error\n");
    buf_code_new = buf_code_index;
    fbuf_save (fbuf, buf_code_list + buf_code_index);
    if (++buf_code_index == 26)
	buf_code_index = 0;
}

static void buf_code_del (const mpeg2_fbuf_t * fbuf)
{
    int i;

    if (fbuf == NULL)
	return;
    for (i = 0; i < 26; i++)
	if (fbuf_match (fbuf, buf_code_list + i)) {
	    buf_code_list[i].ptr = NULL;
	    return;
	}
}

static int picture_match (const mpeg2_picture_t * pic, struct save_pic * saved)
{
    return (saved->ptr == pic && 
	    (pic == NULL ||
	     !memcmp (pic, &saved->value, sizeof (mpeg2_picture_t))));
}

static void picture_save (const mpeg2_picture_t * pic, struct save_pic * saved)
{
    saved->ptr = pic;
    if (pic != NULL)
	saved->value = *pic;
}

static char pic_code (const mpeg2_picture_t * pic)
{
    int i;

    if (pic == NULL)
	return '-';
    for (i = 0; i < 26; i++)
	if (picture_match (pic, pic_code_list + i))
	    return ((i == pic_code_new) ? 'A' : 'a') + i;
    return '?';
}

static void pic_code_add (const mpeg2_picture_t * pic, FILE * f)
{
    int i;

    if (pic == NULL)
	fprintf (f, "pic_code_add error\n");
    for (i = 0; i < 26; i++)
	if (pic_code_list[i].ptr == pic)
	    fprintf (f, "pic_code_add error\n");
    pic_code_new = pic_code_index;
    picture_save (pic, pic_code_list + pic_code_index);
    if (++pic_code_index == 26)
	pic_code_index = 0;
}

static void pic_code_del (const mpeg2_picture_t * pic)
{
    int i;

    if (pic == NULL)
	return;
    for (i = 0; i < 26; i++)
	if (picture_match (pic, pic_code_list + i)) {
	    pic_code_list[i].ptr = NULL;
	    return;
	}
}

void dump_state (FILE * f, mpeg2_state_t state, const mpeg2_info_t * info,
		 int offset, int verbose)
{
    static const char * state_name[] = {
	"BUFFER", "SEQUENCE", "SEQUENCE_REPEATED","GOP",
	"PICTURE", "SLICE_1ST", "PICTURE_2ND", "SLICE", "END",
	"INVALID", "INVALID_END", "SEQUENCE_MODIFIED"
    };
    static const char * profile[] = { "HP", "Spatial", "SNR", "MP", "SP" };
    static const char * level[] = { "HL", "H-14", "ML", "LL" };
    static const char * profile2[] = { "422@HL", NULL, NULL, "422@ML",
				 NULL, NULL, NULL, NULL, "MV@HL",
				 "MV@H-14", NULL, "MV@ML", "MV@LL" };
    static const char * video_fmt[] = { "COMPONENT", "PAL", "NTSC", "SECAM", "MAC"};
    static const char coding_type[] = { '0', 'I', 'P', 'B', 'D', '5', '6', '7'};
    static const char * colour[] = { NULL, "BT.709", "UNSPECIFIED", NULL,
			       "BT.470-2/M", "BT.470-2/B,G",
			       "SMPTE170M", "SMPTE240M", "LINEAR" };
    static const char * colour3[] = { NULL, "BT.709", "UNSPEC_COLORS", NULL, NULL,
				"BT.470-2/B,G", "SMPTE170M", "SMPTE240M" };
    const mpeg2_sequence_t * seq = info->sequence;
    const mpeg2_gop_t * gop = info->gop;
    const mpeg2_picture_t * pic;
    unsigned int i, nb_pos, pixel_width, pixel_height;

    if (state == STATE_BUFFER &&
	sequence_match (seq) && gop_match (gop) &&
	info->user_data == NULL && info->user_data_len == 0 &&
	fbuf_match (info->current_fbuf, &last_curbuf) &&
	fbuf_match (info->display_fbuf, &last_dispbuf) &&
	fbuf_match (info->discard_fbuf, &last_discbuf) &&
	picture_match (info->current_picture, &last_curpic) &&
	picture_match (info->current_picture_2nd, &last_curpic2) &&
	picture_match (info->display_picture, &last_disppic) &&
	picture_match (info->display_picture_2nd, &last_disppic2))
	return;
    fprintf (f, "%8x", offset);
    if (verbose > 1) {
	switch (state) {
	case STATE_PICTURE:
	    buf_code_add (info->current_fbuf, f);
	    pic_code_add (info->current_picture, f);
	    break;
	case STATE_PICTURE_2ND:
	    pic_code_add (info->current_picture_2nd, f);
	    break;
	case STATE_SEQUENCE_MODIFIED:
	    if (last_sequence.value.width != seq->width ||
		last_sequence.value.height != seq->height ||
		last_sequence.value.chroma_width != seq->chroma_width ||
		last_sequence.value.chroma_height != seq->chroma_height ||
		((last_sequence.value.flags & SEQ_FLAG_LOW_DELAY) !=
		 (seq->flags & SEQ_FLAG_LOW_DELAY)))
		fprintf (f, " (INVALID)");
	case STATE_SEQUENCE:
	    sequence_save (seq);
	    break;
	    break;
	case STATE_GOP:
	    gop_save (gop);
	    break;
	default:
	    break;
	}
	fprintf (f, " %c%c %c%c%c %c%c%c %c", seq_code (seq), gop_code (gop),
		 buf_code (info->current_fbuf),
		 pic_code (info->current_picture),
		 pic_code (info->current_picture_2nd),
		 buf_code (info->display_fbuf),
		 pic_code (info->display_picture),
		 pic_code (info->display_picture_2nd),
		 buf_code (info->discard_fbuf));
	if (state == STATE_SLICE || state == STATE_END ||
	    state == STATE_INVALID_END) {
	    if (state != STATE_SLICE)
		buf_code_del (info->display_fbuf);
	    buf_code_del (info->discard_fbuf);
	    pic_code_del (info->display_picture);
	    pic_code_del (info->display_picture_2nd);
	}
	buf_code_new = pic_code_new = -1;
    }
    fprintf (f, " %s", state_name[state]);
    switch (state) {
    case STATE_SEQUENCE:
    case STATE_SEQUENCE_REPEATED:
    case STATE_SEQUENCE_MODIFIED:
	if (seq->flags & SEQ_FLAG_MPEG2)
	    fprintf (f, " MPEG2");
	if (0x10 <= seq->profile_level_id && seq->profile_level_id < 0x60 &&
	    !(seq->profile_level_id & 1) &&
	    4 <= (seq->profile_level_id & 15) &&
	    (seq->profile_level_id & 15) <= 10)
	    fprintf (f, " %s@%s",
		     profile[(seq->profile_level_id >> 4) - 1],
		     level[((seq->profile_level_id & 15) - 4) >> 1]);
	else if (0x82 <= seq->profile_level_id &&
		 seq->profile_level_id<= 0x8e &&
		 profile2[seq->profile_level_id - 0x82])
	    fprintf (f, " %s", profile2[seq->profile_level_id - 0x82]);
	else if (seq->flags & SEQ_FLAG_MPEG2)
	    fprintf (f, " profile %02x", seq->profile_level_id);
	if (seq->flags & SEQ_FLAG_CONSTRAINED_PARAMETERS)
	    fprintf (f, " CONST");
	if (seq->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)
	    fprintf (f, " PROG");
	if (seq->flags & SEQ_FLAG_LOW_DELAY)
	    fprintf (f, " LOWDELAY");
	if ((seq->flags & SEQ_MASK_VIDEO_FORMAT) <
	    SEQ_VIDEO_FORMAT_UNSPECIFIED)
	    fprintf (f, " %s", video_fmt[(seq->flags & SEQ_MASK_VIDEO_FORMAT) /
					 SEQ_VIDEO_FORMAT_PAL]);
	if (seq->flags & SEQ_FLAG_COLOUR_DESCRIPTION) {
	    if (seq->colour_primaries == seq->transfer_characteristics &&
		seq->colour_primaries == seq->matrix_coefficients &&
		seq->colour_primaries <= 7 && colour3[seq->colour_primaries])
		fprintf (f, " %s", colour3[seq->colour_primaries]);
	    else {
		char prim[16], trans[16], matrix[16];
		sprintf (prim, "%d", seq->colour_primaries);
		sprintf (trans, "%d", seq->transfer_characteristics);
		sprintf (matrix, "%d", seq->matrix_coefficients);
		if (seq->colour_primaries <= 7 &&
		    colour[seq->colour_primaries])
		    strncpy (prim, colour[seq->colour_primaries], 15);
		if (seq->transfer_characteristics <= 8 &&
		    colour[seq->transfer_characteristics])
		    strncpy (trans, colour[seq->transfer_characteristics], 15);
		if (seq->matrix_coefficients == 4)
		    strncpy (matrix, "FCC", 15);
		else if (seq->matrix_coefficients <= 7 &&
			 colour[seq->matrix_coefficients])
		    strncpy (matrix, colour[seq->matrix_coefficients], 15);
		fprintf (f, " COLORS (prim %s trans %s matrix %s)",
			 prim, trans, matrix);
	    }
	}
	fprintf (f, " %dx%d chroma %dx%d fps %.*f maxBps %d vbv %d "
		 "picture %dx%d display %dx%d pixel %dx%d",
		 seq->width, seq->height,
		 seq->chroma_width, seq->chroma_height,
		 27000000%seq->frame_period?2:0, 27000000.0/seq->frame_period,
		 seq->byte_rate, seq->vbv_buffer_size,
		 seq->picture_width, seq->picture_height,
		 seq->display_width, seq->display_height,
		 seq->pixel_width, seq->pixel_height);
	if (mpeg2_guess_aspect (seq, &pixel_width, &pixel_height))
	    fprintf (f, " guessed %dx%d", pixel_width, pixel_height);
	fprintf (f, "\n");
	break;
    case STATE_GOP:
	if (gop->flags & GOP_FLAG_DROP_FRAME)
	    fprintf (f, " DROP");
	if (gop->flags & GOP_FLAG_CLOSED_GOP)
	    fprintf (f, " CLOSED");
	if (gop->flags & GOP_FLAG_BROKEN_LINK)
	    fprintf (f, " BROKEN");
	fprintf (f, " %2d:%2d:%2d:%2d\n",
		 gop->hours, gop->minutes, gop->seconds, gop->pictures);
	break;
    case STATE_PICTURE:
    case STATE_PICTURE_2ND:
	pic = ((state == STATE_PICTURE) ?
	       info->current_picture : info->current_picture_2nd);
	fprintf (f, " %c",
		 coding_type[pic->flags & PIC_MASK_CODING_TYPE]);
	if (pic->flags & PIC_FLAG_PROGRESSIVE_FRAME)
	    fprintf (f, " PROG");
	if (pic->flags & PIC_FLAG_SKIP)
	    fprintf (f, " SKIP");
	fprintf (f, " fields %d", pic->nb_fields);
	if (pic->flags & PIC_FLAG_TOP_FIELD_FIRST)
	    fprintf (f, " TFF");
	if (pic->flags & PIC_FLAG_TAGS)
	    fprintf (f, " pts %08x dts %08x", pic->tag, pic->tag2);
	fprintf (f, " time_ref %d", pic->temporal_reference);
	if (pic->flags & PIC_FLAG_COMPOSITE_DISPLAY)
	    fprintf (f, " composite %05x", pic->flags >> 12);
	fprintf (f, " offset");
	nb_pos = pic->nb_fields;
	if (seq->flags & SEQ_FLAG_PROGRESSIVE_SEQUENCE)
	    nb_pos >>= 1;
	for (i = 0; i < nb_pos; i++)
	    fprintf (f, " %d/%d",
		     pic->display_offset[i].x, pic->display_offset[i].y);
	fprintf (f, "\n");
	break;
    default:
	fprintf (f, "\n");
    }
    if (verbose > 2 && info->user_data_len) {
	fprintf (f, "         USER_DATA %d bytes\n", info->user_data_len);
	if (verbose > 3)
	    for (i = 0; i < info->user_data_len; i += 16) {
		unsigned int j;

		fprintf (f, "         ");
		for (j = i; j < i + 16; j++)
		    if (j < info->user_data_len)
			fprintf (f, "%02x ", info->user_data[j]);
		    else
			fprintf (f, "   ");
		fprintf (f, " ");
		for (j = i; j < i + 16; j++)
		    if (j < info->user_data_len &&
			32 <= info->user_data[j] && info->user_data[j] <= 126)
			fprintf (f, "%c", info->user_data[j]);
		    else
			fprintf (f, " ");
		fprintf (f, "\n");
	    }
    }
    if (state == STATE_END || state == STATE_INVALID_END) {
	sequence_save (NULL);
	gop_save (NULL);
	fbuf_save (NULL, &last_curbuf);
	fbuf_save (NULL, &last_dispbuf);
	fbuf_save (NULL, &last_discbuf);
	picture_save (NULL, &last_curpic);
	picture_save (NULL, &last_curpic2);
	picture_save (NULL, &last_disppic);
	picture_save (NULL, &last_disppic2);
    } else {
	sequence_save (seq);
	gop_save (gop);
	fbuf_save (info->current_fbuf, &last_curbuf);
	fbuf_save (info->display_fbuf, &last_dispbuf);
	fbuf_save (info->discard_fbuf, &last_discbuf);
	picture_save (info->current_picture, &last_curpic);
	picture_save (info->current_picture_2nd, &last_curpic2);
	picture_save (info->display_picture, &last_disppic);
	picture_save (info->display_picture_2nd, &last_disppic2);
    }
}

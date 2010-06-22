/*
 * This file is part of Media Player Classic HomeCinema.
 *
 * MPC-HC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

//static int vc1_decode_frame(AVCodecContext *avctx,
//                            void *data, int *data_size,
//                            uint8_t *buf, int buf_size)
int av_vc1_decode_frame(AVCodecContext *avctx,
                            uint8_t *buf, int buf_size)
{
    VC1Context *v = avctx->priv_data;
    MpegEncContext *s = &v->s;
    //AVFrame *pict = data;
    uint8_t *buf2 = NULL;

	/*v->allow_interlaced = 1;*/
	v->lumshift			= 0;
	v->lumscale			= 32;

    /* We need to set current_picture_ptr before reading the header,
     * otherwise we cannot store anything in there. */
    if(s->current_picture_ptr==NULL || s->current_picture_ptr->data[0]){
        int i= ff_find_unused_picture(s, 0);
        s->current_picture_ptr= &s->picture[i];
    }

    //for advanced profile we may need to parse and unescape data
    if (avctx->codec_id == CODEC_ID_VC1) {
        int buf_size2 = 0;
        buf2 = av_mallocz(buf_size + FF_INPUT_BUFFER_PADDING_SIZE);

        if(IS_MARKER(AV_RB32(buf))){ /* frame starts with marker and needs to be parsed */
            const uint8_t *start, *end, *next;
            int size;

            next = buf;
            for(start = buf, end = buf + buf_size; next < end; start = next){
                next = find_next_marker(start + 4, end);
                size = next - start - 4;
                if(size <= 0) continue;
                switch(AV_RB32(start)){
                case VC1_CODE_FRAME:
                    buf_size2 = vc1_unescape_buffer(start + 4, size, buf2);
                    break;
                case VC1_CODE_ENTRYPOINT: /* it should be before frame data */
                    buf_size2 = vc1_unescape_buffer(start + 4, size, buf2);
                    init_get_bits(&s->gb, buf2, buf_size2*8);
                    vc1_decode_entry_point(avctx, v, &s->gb);
                    break;
                case VC1_CODE_SLICE:
                    av_log(avctx, AV_LOG_ERROR, "Sliced decoding is not implemented (yet)\n");
                    av_free(buf2);
                    return -1;
                }
            }
        }else if(v->interlace && ((buf[0] & 0xC0) == 0xC0)){ /* WVC1 interlaced stores both fields divided by marker */
            const uint8_t *divider;

            divider = find_next_marker(buf, buf + buf_size);
            if((divider == (buf + buf_size)) || AV_RB32(divider) != VC1_CODE_FIELD){
                av_log(avctx, AV_LOG_ERROR, "Error in WVC1 interlaced frame\n");
				av_free(buf2);
                return -1;
            }

            buf_size2 = vc1_unescape_buffer(buf, divider - buf, buf2);
            // TODO
            av_free(buf2);return -1;
        }else{
            buf_size2 = vc1_unescape_buffer(buf, buf_size, buf2);
        }
        init_get_bits(&s->gb, buf2, buf_size2*8);
    } else
        init_get_bits(&s->gb, buf, buf_size*8);
    // do parse frame header
    if(v->profile < PROFILE_ADVANCED) {
        if(vc1_parse_frame_header(v, &s->gb) == -1) {
            av_free(buf2);
            return -1;
        }
    } else {
        if(vc1_parse_frame_header_adv(v, &s->gb) == -1) {
            av_free(buf2);
            return -1;
        }
    }
    av_free(buf2);
    return 0;
}
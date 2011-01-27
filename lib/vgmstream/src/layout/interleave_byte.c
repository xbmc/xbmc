#include "layout.h"
#include "../vgmstream.h"

/* for formats where the interleave is smaller than a frame, so we need to
 * deinterleave in memory before passing it along to a specialized decoder which
 * reads from memory
 */

/* just do one frame at a time */

void render_vgmstream_interleave_byte(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    /* frame sizes are much smaller than this */
    uint8_t sample_data[0x400];
    int samples_written=0;

    int frame_size = get_vgmstream_frame_size(vgmstream);
    int samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);
    int samples_this_block;

    samples_this_block = samples_per_frame;

    while (samples_written<sample_count) {
        int samples_to_do; 

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            continue;
        }

        samples_to_do = vgmstream_samples_to_do(samples_this_block, samples_per_frame, vgmstream);

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        {
            int i,j;
            for (j=0;j<vgmstream->channels;j++) {
                for (i=0;i<frame_size;i++) {
                    sample_data[i] = read_8bit(vgmstream->ch[j].offset+
                            i/vgmstream->interleave_block_size*
                            vgmstream->interleave_block_size*
                            vgmstream->channels+
                            i%vgmstream->interleave_block_size,
                            vgmstream->ch[j].streamfile);
                }
                decode_vgmstream_mem(vgmstream, samples_written,
                        samples_to_do, buffer, sample_data, j);
            }
        }

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;

        if (vgmstream->samples_into_block==samples_this_block) {
            int chan;
            for (chan=0;chan<vgmstream->channels;chan++)
                vgmstream->ch[chan].offset+=frame_size*vgmstream->channels;
            vgmstream->samples_into_block=0;
        }

    }
}

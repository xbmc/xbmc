#include "layout.h"
#include "../vgmstream.h"

void render_vgmstream_interleave(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;

    int frame_size = get_vgmstream_frame_size(vgmstream);
    int samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);
    int samples_this_block;

    samples_this_block = vgmstream->interleave_block_size / frame_size * samples_per_frame;

    if (vgmstream->layout_type == layout_interleave_shortblock &&
        vgmstream->current_sample - vgmstream->samples_into_block + samples_this_block> vgmstream->num_samples) {
        frame_size = get_vgmstream_shortframe_size(vgmstream);
        samples_per_frame = get_vgmstream_samples_per_shortframe(vgmstream);

        samples_this_block = vgmstream->interleave_smallblock_size / frame_size * samples_per_frame;
    }

    while (samples_written<sample_count) {
        int samples_to_do; 

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            /* we assume that the loop is not back into a short block */
            if (vgmstream->layout_type == layout_interleave_shortblock) {
                frame_size = get_vgmstream_frame_size(vgmstream);
                samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);
                samples_this_block = vgmstream->interleave_block_size / frame_size * samples_per_frame;
            }
            continue;
        }

        samples_to_do = vgmstream_samples_to_do(samples_this_block, samples_per_frame, vgmstream);
        /*printf("vgmstream_samples_to_do(samples_this_block=%d,samples_per_frame=%d,vgmstream) returns %d\n",samples_this_block,samples_per_frame,samples_to_do);*/

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        decode_vgmstream(vgmstream, samples_written, samples_to_do, buffer);

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;

        if (vgmstream->samples_into_block==samples_this_block) {
            int chan;
            if (vgmstream->layout_type == layout_interleave_shortblock &&
                vgmstream->current_sample + samples_this_block > vgmstream->num_samples) {
                frame_size = get_vgmstream_shortframe_size(vgmstream);
                samples_per_frame = get_vgmstream_samples_per_shortframe(vgmstream);

                samples_this_block = vgmstream->interleave_smallblock_size / frame_size * samples_per_frame;
                for (chan=0;chan<vgmstream->channels;chan++)
                    vgmstream->ch[chan].offset+=vgmstream->interleave_block_size*(vgmstream->channels-chan)+vgmstream->interleave_smallblock_size*chan;
            } else {

                for (chan=0;chan<vgmstream->channels;chan++)
                    vgmstream->ch[chan].offset+=vgmstream->interleave_block_size*vgmstream->channels;
            }
            vgmstream->samples_into_block=0;
        }

    }
}

#include "layout.h"
#include "../vgmstream.h"

void render_vgmstream_nolayout(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;

    const int samples_this_block = vgmstream->num_samples;
    int samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);

    while (samples_written<sample_count) {
        int samples_to_do;

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            continue;
        }

        samples_to_do = vgmstream_samples_to_do(samples_this_block, samples_per_frame, vgmstream);

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        decode_vgmstream(vgmstream, samples_written, samples_to_do, buffer);

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;
    }
}

#include "layout.h"
#include "../vgmstream.h"
#include "../coding/acm_decoder.h"
#include "../coding/coding.h"

void render_vgmstream_mus_acm(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;
    mus_acm_codec_data *data = vgmstream->codec_data;

    while (samples_written<sample_count) {
        ACMStream *acm = data->files[data->current_file];
        int samples_to_do;
        int samples_this_block = acm->total_values / acm->info.channels;

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            data->current_file = data->loop_start_file;
            acm_reset(data->files[data->current_file]);
            vgmstream->samples_into_block = 0;
            continue;
        }

        samples_to_do = vgmstream_samples_to_do(samples_this_block, 1, vgmstream);

        /*printf("samples_to_do=%d,samples_this_block=%d,samples_written=%d,sample_count=%d\n",samples_to_do,samples_this_block,samples_written,sample_count);*/

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        if (samples_to_do == 0)
        {
            data->current_file++;
            /*printf("next %d, %d samples\n",data->current_file,data->files[data->current_file]->total_values/data->files[data->current_file]->info.channels);*/
            /* force loop back to first file in case we're still playing for some
             * reason, prevent out of bounds stuff */
            if (data->current_file >= data->file_count) data->current_file = 0;
            acm_reset(data->files[data->current_file]);
            vgmstream->samples_into_block = 0;
            continue;
        }

        /*printf("decode %d samples file %d\n",samples_to_do,data->current_file);*/
        decode_acm(acm,
                buffer+samples_written*vgmstream->channels,
                samples_to_do, vgmstream->channels);

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;
    }
}

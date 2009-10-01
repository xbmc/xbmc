#include "layout.h"
#include "../vgmstream.h"
#include "../coding/coding.h"

void render_vgmstream_aax(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;
    aax_codec_data *data = vgmstream->codec_data;

    while (samples_written<sample_count) {
        int samples_to_do;
        int samples_this_block = data->sample_counts[data->current_segment];

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            int i;
            data->current_segment = data->loop_segment;

            reset_vgmstream(data->adxs[data->current_segment]);

            /* carry over the history from the loop point */
            if (data->loop_segment > 0)
            {
                for (i=0;i<data->adxs[0]->channels;i++)
                {
                    data->adxs[data->loop_segment]->ch[i].adpcm_history1_32 = 
                        data->adxs[data->loop_segment-1]->ch[i].adpcm_history1_32;
                    data->adxs[data->loop_segment]->ch[i].adpcm_history2_32 = 
                        data->adxs[data->loop_segment-1]->ch[i].adpcm_history2_32;
                }
            }
            vgmstream->samples_into_block = 0;
            continue;
        }

        samples_to_do = vgmstream_samples_to_do(samples_this_block, 1, vgmstream);

        /*printf("samples_to_do=%d,samples_this_block=%d,samples_written=%d,sample_count=%d\n",samples_to_do,samples_this_block,samples_written,sample_count);*/

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        if (samples_to_do == 0)
        {
            int i;
            data->current_segment++;
            /*printf("advance to %d at %d samples\n",data->current_segment,vgmstream->current_sample);*/
            reset_vgmstream(data->adxs[data->current_segment]);

            /* carry over the history from the previous segment */
            for (i=0;i<data->adxs[0]->channels;i++)
            {
                data->adxs[data->current_segment]->ch[i].adpcm_history1_32 = 
                    data->adxs[data->current_segment-1]->ch[i].adpcm_history1_32;
                data->adxs[data->current_segment]->ch[i].adpcm_history2_32 = 
                    data->adxs[data->current_segment-1]->ch[i].adpcm_history2_32;
            }
            vgmstream->samples_into_block = 0;
            continue;
        }

        render_vgmstream(&buffer[samples_written*data->adxs[data->current_segment]->channels],
                samples_to_do,data->adxs[data->current_segment]);

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;
    }
}

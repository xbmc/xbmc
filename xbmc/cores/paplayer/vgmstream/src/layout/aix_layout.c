#include "layout.h"
#include "../vgmstream.h"
#include "../coding/coding.h"

void render_vgmstream_aix(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;
    aix_codec_data *data = vgmstream->codec_data;

    while (samples_written<sample_count) {
        int samples_to_do;
        int samples_this_block = data->sample_counts[data->current_segment];
        int current_stream;
        int channels_sofar = 0;

        if (vgmstream->loop_flag && vgmstream_do_loop(vgmstream)) {
            data->current_segment = 1;
            for (current_stream = 0; current_stream < data->stream_count; current_stream++)
            {
                int i;
                reset_vgmstream(data->adxs[data->current_segment*data->stream_count+current_stream]);

                /* carry over the history from the loop point */
                for (i=0;i<data->adxs[data->stream_count+current_stream]->channels;i++)
                {
                    data->adxs[1*data->stream_count+current_stream]->ch[i].adpcm_history1_32 = 
                        data->adxs[0+current_stream]->ch[i].adpcm_history1_32;
                    data->adxs[1*data->stream_count+current_stream]->ch[i].adpcm_history2_32 = 
                        data->adxs[0+current_stream]->ch[i].adpcm_history2_32;
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
            /*printf("next %d, %d samples\n",data->current_file,data->files[data->current_file]->total_values/data->files[data->current_file]->info.channels);*/
            for (current_stream = 0; current_stream < data->stream_count; current_stream++)
            {
                reset_vgmstream(data->adxs[data->current_segment*data->stream_count+current_stream]);

                /* carry over the history from the previous segment */
                for (i=0;i<data->adxs[data->current_segment*data->stream_count+current_stream]->channels;i++)
                {
                    data->adxs[data->current_segment*data->stream_count+current_stream]->ch[i].adpcm_history1_32 = 
                        data->adxs[(data->current_segment-1)*data->stream_count+current_stream]->ch[i].adpcm_history1_32;
                    data->adxs[data->current_segment*data->stream_count+current_stream]->ch[i].adpcm_history2_32 = 
                        data->adxs[(data->current_segment-1)*data->stream_count+current_stream]->ch[i].adpcm_history2_32;
                }
            }
            vgmstream->samples_into_block = 0;
            continue;
        }

        /*printf("decode %d samples file %d\n",samples_to_do,data->current_file);*/
        if (samples_to_do > AIX_BUFFER_SIZE/2)
        {
            samples_to_do = AIX_BUFFER_SIZE/2;
        }

        for (current_stream = 0; current_stream < data->stream_count; current_stream++)
        {
            int i,j;
            VGMSTREAM *adx = data->adxs[data->current_segment*data->stream_count+current_stream];

            render_vgmstream(data->buffer,samples_to_do,adx);

            for (i = 0; i < samples_to_do; i++)
            {
                for (j = 0; j < adx->channels; j++)
                {
                    buffer[(i+samples_written)*vgmstream->channels+channels_sofar+j] = data->buffer[i*adx->channels+j];
                }
            }

            channels_sofar += adx->channels;
        }

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;
    }
}

#include "../vgmstream.h"

#ifdef VGM_USE_VORBIS
#include <vorbis/vorbisfile.h>
#include "coding.h"
#include "../util.h"

void decode_ogg_vorbis(ogg_vorbis_codec_data * data, sample * outbuf, int32_t samples_to_do, int channels) {
    int samples_done = 0;
    OggVorbis_File *ogg_vorbis_file = &data->ogg_vorbis_file;

    do {
        long rc = ov_read(ogg_vorbis_file, (char *)(outbuf + samples_done*channels),
                (samples_to_do - samples_done)*sizeof(sample)*channels, 0,
                sizeof(sample), 1, &data->bitstream);

        if (rc > 0) samples_done += rc/sizeof(sample)/channels;
        else return;
    } while (samples_done < samples_to_do);
}

#endif

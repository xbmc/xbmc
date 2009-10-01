#include "../vgmstream.h"

#ifdef VGM_USE_MPEG
#include <string.h>
#include <mpg123.h>
#include "coding.h"
#include "../util.h"

/* mono, mpg123 expects frames of 0x414 (160kbps, 22050Hz) but they
 * actually vary and are much shorter */
void decode_fake_mpeg2_l2(VGMSTREAMCHANNEL *stream,
        mpeg_codec_data * data,
        sample * outbuf, int32_t samples_to_do) {
    int samples_done = 0;

    while (samples_done < samples_to_do) {
        size_t bytes_done;
        int rc;

        if (!data->buffer_full) {
            /* fill buffer up to next frame ending (or file ending) */
            int bytes_into_header = 0;
            const uint8_t header[4] = {0xff,0xf5,0xe0,0xc0};
            off_t frame_offset = 0;

            /* assume that we are starting at a header, skip it and look for the
             * next one */
            read_streamfile(data->buffer, stream->offset+frame_offset, 4,
                    stream->streamfile);
            frame_offset += 4;

            do {
                uint8_t byte;
                byte =
                    read_8bit(stream->offset+frame_offset,stream->streamfile);
                data->buffer[frame_offset] = byte;
                frame_offset++;

                if (byte == header[bytes_into_header]) {
                    bytes_into_header++;
                } else {
                    /* This might have been the first byte of the header, so
                     * we need to check again.
                     * No need to get more complicated than this, though, since
                     * there are no repeated characters in the search string. */
                    if (bytes_into_header>0) {
                        frame_offset--;
                    }
                    bytes_into_header=0;
                }

                if (bytes_into_header==4) {
                    break;
                }
            } while (frame_offset < AHX_EXPECTED_FRAME_SIZE);

            if (bytes_into_header==4) frame_offset-=4;
            memset(data->buffer+frame_offset,0,
                    AHX_EXPECTED_FRAME_SIZE-frame_offset);

            data->buffer_full = 1;
            data->buffer_used = 0;

            stream->offset += frame_offset;
        }

        if (!data->buffer_used) {
            rc = mpg123_decode(data->m,
                    data->buffer,AHX_EXPECTED_FRAME_SIZE,
                    (unsigned char *)(outbuf+samples_done),
                    (samples_to_do-samples_done)*sizeof(sample),
                    &bytes_done);
            data->buffer_used = 1;
        } else {
            rc = mpg123_decode(data->m,
                    NULL,0,
                    (unsigned char *)(outbuf+samples_done),
                    (samples_to_do-samples_done)*sizeof(sample),
                    &bytes_done);
        }

        if (rc == MPG123_NEED_MORE) data->buffer_full = 0;

        samples_done += bytes_done/sizeof(sample);
    }
}

/* decode anything mpg123 can */
void decode_mpeg(VGMSTREAMCHANNEL *stream,
        mpeg_codec_data * data,
        sample * outbuf, int32_t samples_to_do, int channels) {
    int samples_done = 0;

    while (samples_done < samples_to_do) {
        size_t bytes_done;
        int rc;

        if (!data->buffer_full) {
            data->bytes_in_buffer = read_streamfile(data->buffer,
                    stream->offset,MPEG_BUFFER_SIZE,stream->streamfile);

            data->buffer_full = 1;
            data->buffer_used = 0;

            stream->offset += data->bytes_in_buffer;
        }

        if (!data->buffer_used) {
            rc = mpg123_decode(data->m,
                    data->buffer,data->bytes_in_buffer,
                    (unsigned char *)(outbuf+samples_done*channels),
                    (samples_to_do-samples_done)*sizeof(sample)*channels,
                    &bytes_done);
            data->buffer_used = 1;
        } else {
            rc = mpg123_decode(data->m,
                    NULL,0,
                    (unsigned char *)(outbuf+samples_done*channels),
                    (samples_to_do-samples_done)*sizeof(sample)*channels,
                    &bytes_done);
        }

        if (rc == MPG123_NEED_MORE) data->buffer_full = 0;

        samples_done += bytes_done/sizeof(sample)/channels;
    }
}

#endif

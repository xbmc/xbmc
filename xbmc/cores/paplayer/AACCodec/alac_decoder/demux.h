#ifndef DEMUX_H
#define DEMUX_H

//#include <stdint.h>
#include "stream.h"

typedef unsigned int fourcc_t;

typedef struct
{
    unsigned short num_channels;
    unsigned short sample_size;
    unsigned int sample_rate;
    fourcc_t format;
    void *buf;

    struct {
        int sample_count;
        unsigned int sample_duration;
    } *time_to_sample;
    unsigned int num_time_to_samples;

    unsigned int *sample_byte_size;
    unsigned int num_sample_byte_sizes;

    unsigned int codecdata_len;
    void *codecdata;

    unsigned int mdat_len;
#if 0
    void *mdat;
#endif
} demux_res_t;

int qtmovie_read(stream_t *stream, demux_res_t *demux_res);

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ( \
    ( (int)(char)(ch0) << 24 ) | \
    ( (int)(char)(ch1) << 16 ) | \
    ( (int)(char)(ch2) << 8 ) | \
    ( (int)(char)(ch3) ) )
#endif

#ifndef SLPITFOURCC
/* splits it into ch0, ch1, ch2, ch3 - use for printf's */
#define SPLITFOURCC(code) \
    (char)((int)code >> 24), \
    (char)((int)code >> 16), \
    (char)((int)code >> 8), \
    (char)code
#endif

#endif /* DEMUX_H */


#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include "meta.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_adx(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    off_t stream_offset;
    size_t filesize;
    uint16_t version_signature;
    int loop_flag=0;
    int channel_count;
    int32_t loop_start_offset=0;
    int32_t loop_end_offset=0;
    int32_t loop_start_sample=0;
    int32_t loop_end_sample=0;
    meta_t header_type;
    int16_t coef1, coef2;
    uint16_t cutoff;
    char filename[260];

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("adx",filename_extension(filename))) goto fail;

    filesize = get_streamfile_size(streamFile);

    /* check first 2 bytes */
    if ((uint16_t)read_16bitBE(0,streamFile)!=0x8000) goto fail;

    /* get stream offset, check for CRI signature just before */
    stream_offset = (uint16_t)read_16bitBE(2,streamFile) + 4;
    if ((uint16_t)read_16bitBE(stream_offset-6,streamFile)!=0x2863 ||/* "(c" */
        (uint32_t)read_32bitBE(stream_offset-4,streamFile)!=0x29435249 /* ")CRI" */
       ) goto fail;

    /* check for encoding type */
    /* 2 is for some unknown fixed filter, 3 is standard ADX, 4 is
     * ADX with exponential scale, 0x11 is AHX */
    if (read_8bit(4,streamFile) != 3) goto fail;

    /* check for frame size (only 18 is supported at the moment) */
    if (read_8bit(5,streamFile) != 18) goto fail;

    /* check for bits per sample? (only 4 makes sense for ADX) */
    if (read_8bit(6,streamFile) != 4) goto fail;

    /* check version signature, read loop info */
    version_signature = read_16bitBE(0x12,streamFile);
    if (version_signature == 0x0300) {      /* type 03 */
        header_type = meta_ADX_03;
        if (stream_offset-6 >= 0x2c) {   /* enough space for loop info? */
            loop_flag = (read_32bitBE(0x18,streamFile) != 0);
            loop_start_sample = read_32bitBE(0x1c,streamFile);
            loop_start_offset = read_32bitBE(0x20,streamFile);
            loop_end_sample = read_32bitBE(0x24,streamFile);
            loop_end_offset = read_32bitBE(0x28,streamFile);
        }
    } else if (version_signature == 0x0400) {

        off_t	ainf_info_length=0;

        if((uint32_t)read_32bitBE(0x24,streamFile)==0x41494E46) /* AINF Header */
            ainf_info_length = (off_t)read_32bitBE(0x28,streamFile);

        header_type = meta_ADX_04;
        if (stream_offset-ainf_info_length-6 >= 0x38) {   /* enough space for loop info? */
            loop_flag = (read_32bitBE(0x24,streamFile) != 0);
            loop_start_sample = read_32bitBE(0x28,streamFile);
            loop_start_offset = read_32bitBE(0x2c,streamFile);
            loop_end_sample = read_32bitBE(0x30,streamFile);
            loop_end_offset = read_32bitBE(0x34,streamFile);
        }
    } else if (version_signature == 0x0500) {			 /* found in some SFD : Buggy Heat, appears to have no loop */
        header_type = meta_ADX_05;
    } else goto fail;   /* not a known/supported version signature */

    /* At this point we almost certainly have an ADX file,
     * so let's build the VGMSTREAM. */

    /* high-pass cutoff frequency, always 500 that I've seen */
    cutoff = (uint16_t)read_16bitBE(0x10,streamFile);

    channel_count = read_8bit(7,streamFile);
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = read_32bitBE(0xc,streamFile);
    vgmstream->sample_rate = read_32bitBE(8,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    vgmstream->loop_start_sample = loop_start_sample;
    vgmstream->loop_end_sample = loop_end_sample;

    vgmstream->coding_type = coding_CRI_ADX;
    if (channel_count==1)
        vgmstream->layout_type = layout_none;
    else
        vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = header_type;

    vgmstream->interleave_block_size=18;

    /* calculate filter coefficients */
    {
        double x,y,z,a,b,c;

        x = cutoff;
        y = vgmstream->sample_rate;
        z = cos(2.0*M_PI*x/y);

        a = M_SQRT2-z;
        b = M_SQRT2-1.0;
        c = (a-sqrt((a+b)*(a-b)))/b;

        coef1 = floor(c*8192);
        coef2 = floor(c*c*-4096);
    }

    {
        int i;
        STREAMFILE * chstreamfile;
       
        /* ADX is so tightly interleaved that having two buffers is silly */
        chstreamfile = streamFile->open(streamFile,filename,18*0x400);
        if (!chstreamfile) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = chstreamfile;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                stream_offset+18*i;

            vgmstream->ch[i].adpcm_coef[0] = coef1;
            vgmstream->ch[i].adpcm_coef[1] = coef2;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

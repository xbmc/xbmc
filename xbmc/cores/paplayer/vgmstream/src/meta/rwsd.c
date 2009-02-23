#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

static off_t read_rwav(off_t offset, int *version, off_t *start_offset, off_t *info_chunkp, STREAMFILE *streamFile)
{
    off_t info_chunk;
    off_t data_chunk;
    off_t wave_offset;

    if ((uint32_t)read_32bitBE(offset,streamFile)!=0x52574156) /* "RWAV" */
        goto fail;
    if ((uint32_t)read_32bitBE(offset+4,streamFile)!=0xFEFF0102) /* version 2 */
        goto fail;

    info_chunk = offset+read_32bitBE(offset+0x10,streamFile);
    if ((uint32_t)read_32bitBE(info_chunk,streamFile)!=0x494e464f) /* "INFO" */
        goto fail;
    data_chunk = offset+read_32bitBE(offset+0x18,streamFile);
    if ((uint32_t)read_32bitBE(data_chunk,streamFile)!=0x44415441) /* "DATA" */
        goto fail;

    *start_offset = data_chunk + 8;
    *info_chunkp = info_chunk + 8;
    *version = 2;
    wave_offset = info_chunk - 8;

    return wave_offset;
fail:
    return -1;
}

static off_t read_rwar(off_t offset, int *version, off_t *start_offset, off_t *info_chunk, STREAMFILE *streamFile)
{
    off_t wave_offset;
    if ((uint32_t)read_32bitBE(offset,streamFile)!=0x52574152) /* "RWAR" */
        goto fail;
    if ((uint32_t)read_32bitBE(offset+4,streamFile)!=0xFEFF0100) /* version 0 */
        goto fail;

    wave_offset = read_rwav(offset+0x60,version,start_offset,info_chunk,streamFile);
    *version = 0;
    return wave_offset;

fail:
    return -1;
}

/* RWSD is quite similar to BRSTM, but can contain several streams.
 * Still, some games use it for single streams. We only support the
 * single stream form here */
VGMSTREAM * init_vgmstream_rwsd(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    coding_t coding_type;

    off_t info_chunk;
    off_t wave_offset;
    size_t wave_length;
    int codec_number;
    int channel_count;
    int loop_flag;
    int rwar = 0;
    int rwav = 0;
    int version = -1;

    off_t start_offset = 0;
    size_t stream_size;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("rwsd",filename_extension(filename)))
    {
        if (strcasecmp("rwar",filename_extension(filename)))
        {
            if (strcasecmp("rwav",filename_extension(filename)))
            {
                goto fail;
            }
            else
            {
                rwav = 1;
            }
        }
        else
        {
            rwar = 1;
        }
    }

    /* check header */
    if (rwar)
    {
        wave_offset = read_rwar(0,&version,&start_offset,&info_chunk,streamFile);
        if (wave_offset < 0) goto fail;
    }
    else if (rwav)
    {
        wave_offset = read_rwav(0,&version,&start_offset,&info_chunk,streamFile);
        if (wave_offset < 0) goto fail;
    }
    else
    {
        if ((uint32_t)read_32bitBE(0,streamFile)!=0x52575344) /* "RWSD" */
            goto fail;

        switch (read_32bitBE(4,streamFile))
        {
            case 0xFEFF0102:
                /* ideally we would look through the chunk list for a WAVE chunk,
                 * but it's always in the same order */
                /* get WAVE offset, check */
                wave_offset = read_32bitBE(0x18,streamFile);
                if ((uint32_t)read_32bitBE(wave_offset,streamFile)!=0x57415645) /* "WAVE" */
                    goto fail;
                /* get WAVE size, check */
                wave_length = read_32bitBE(0x1c,streamFile);
                if (read_32bitBE(wave_offset+4,streamFile)!=wave_length)
                    goto fail;

                /* check wave count */
                if (read_32bitBE(wave_offset+8,streamFile) != 1)
                    goto fail; /* only support 1 */

                version = 2;

                break;
            case 0xFEFF0103:
                wave_offset = read_rwar(0xe0,&version,&start_offset,&info_chunk,streamFile);
                if (wave_offset < 0) goto fail;

                rwar = 1;
                break;
            default:
                goto fail;
        }

    }

    /* get type details */
    codec_number = read_8bit(wave_offset+0x10,streamFile);
    loop_flag = read_8bit(wave_offset+0x11,streamFile);
    channel_count = read_8bit(wave_offset+0x12,streamFile);

    switch (codec_number) {
        case 0:
            coding_type = coding_PCM8;
            break;
        case 1:
            coding_type = coding_PCM16BE;
            break;
        case 2:
            coding_type = coding_NGC_DSP;
            break;
        default:
            goto fail;
    }

    if (channel_count < 1) goto fail;

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = dsp_nibbles_to_samples(read_32bitBE(wave_offset+0x1c,streamFile));
    vgmstream->sample_rate = (uint16_t)read_16bitBE(wave_offset+0x14,streamFile);
    /* channels and loop flag are set by allocate_vgmstream */
    vgmstream->loop_start_sample = dsp_nibbles_to_samples(read_32bitBE(wave_offset+0x18,streamFile));
    vgmstream->loop_end_sample = vgmstream->num_samples;

    vgmstream->coding_type = coding_type;
    vgmstream->layout_type = layout_none;

    if (rwar)
        vgmstream->meta_type = meta_RWAR;
    else if (rwav)
        vgmstream->meta_type = meta_RWAV;
    else
        vgmstream->meta_type = meta_RWSD;

    if (vgmstream->coding_type == coding_NGC_DSP) {
        off_t coef_offset;
        int i,j;

        for (j=0;j<vgmstream->channels;j++) {
            if (rwar || rwav)
            {
                /* This is pretty nasty, so an explaination is in order.
                 * At 0x10 in the info_chunk is the offset of a table with
                 * one entry per channel. Each entry in this table is itself
                 * an offset to a set of information for the channel. The
                 * first element in the set is the offset into DATA of the
                 * channel. The stream_size read far below just happens to
                 * hit on this properly for stereo. The second element is the
                 * offset of the coefficient table for the channel. */
                coef_offset = info_chunk + 
                    read_32bitBE(info_chunk + 
                            read_32bitBE(info_chunk+
                                read_32bitBE(info_chunk+0x10,streamFile)+j*4,
                                streamFile) + 4, streamFile);
            } else {
                coef_offset=wave_offset+0x6c+j*0x30;
            }
            for (i=0;i<16;i++) {
                vgmstream->ch[j].adpcm_coef[i]=read_16bitBE(coef_offset+i*2,streamFile);
            }
        }
    }

    if (rwar || rwav)
    {
        /* */
    }
    else
    {
        if (version == 2)
            start_offset = read_32bitBE(8,streamFile);
    }
    stream_size = read_32bitBE(wave_offset+0x50,streamFile);

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,
                    0x1000);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                start_offset + i*stream_size;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

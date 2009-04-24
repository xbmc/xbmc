#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* Resource Interchange File Format */
/* only the bare minimum needed to read PCM wavs */

/* return milliseconds */
long parse_marker(unsigned char * marker) {
    long hh,mm,ss,ms;
    if (memcmp("Marker ",marker,7)) return -1;

    if (4 != sscanf((char*)marker+7,"%ld:%ld:%ld.%ld",&hh,&mm,&ss,&ms))
        return -1;

    return ((hh*60+mm)*60+ss)*1000+ms;
}

/* loop points have been found hiding here */
void parse_adtl(off_t adtl_offset, off_t adtl_length, STREAMFILE  *streamFile,
        long *loop_start, long *loop_end, int *loop_flag) {
    int loop_start_found = 0;
    int loop_end_found = 0;

    off_t current_chunk = adtl_offset+4;

    while (current_chunk < adtl_offset+adtl_length) {
        uint32_t chunk_type = read_32bitBE(current_chunk,streamFile);
        off_t chunk_size = read_32bitLE(current_chunk+4,streamFile);

        if (current_chunk+8+chunk_size > adtl_offset+adtl_length) return;

        switch(chunk_type) {
            case 0x6c61626c:    /* labl */
                {
                    unsigned char *labelcontent;
                    labelcontent = malloc(chunk_size-4);
                    if (!labelcontent) return;
                    if (read_streamfile(labelcontent,current_chunk+0xc,
                                chunk_size-4,streamFile)!=chunk_size-4) {
                        free(labelcontent);
                        return;
                    }

                    switch (read_32bitLE(current_chunk+8,streamFile)) {
                        case 1:
                            if (!loop_start_found &&
                                (*loop_start = parse_marker(labelcontent))>=0)
                            {
                                loop_start_found = 1;
                            }
                            break;
                        case 2:
                            if (!loop_end_found &&
                                    (*loop_end = parse_marker(labelcontent))>=0)
                            {
                                loop_end_found = 1;
                            }
                            break;
                        default:
                            break;
                    }

                    free(labelcontent);
                }
                break;
            default:
                break;
        }

        current_chunk += 8 + chunk_size;
    }

    if (loop_start_found && loop_end_found) *loop_flag = 1;

    /* labels don't seem to be consistently ordered */
    if (*loop_start > *loop_end) {
        long temp = *loop_start;
        *loop_start = *loop_end;
        *loop_end = temp;
    }
}

VGMSTREAM * init_vgmstream_riff(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    off_t file_size = -1;
    int channel_count = 0;
    int sample_count = 0;
    int sample_rate = 0;
    int coding_type = -1;
    off_t start_offset = -1;
    int interleave = -1;

    int loop_flag = 0;
    long loop_start_ms = -1;
    long loop_end_ms = -1;
    off_t loop_start_offset = -1;
    off_t loop_end_offset = -1;
    uint32_t riff_size;
    uint32_t data_size = 0;

    int FormatChunkFound = 0;
    int DataChunkFound = 0;

    /* Level-5 mwv */
    int mwv = 0;
    off_t mwv_pflt_offset = -1;
    off_t mwv_ctrl_offset = -1;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("wav",filename_extension(filename)) &&
        strcasecmp("lwav",filename_extension(filename)))
    {
        if (strcasecmp("mwv",filename_extension(filename)))
            goto fail;
        else
            mwv = 1;
    }

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x52494646) /* "RIFF" */
        goto fail;
    /* check for WAVE form */
    if ((uint32_t)read_32bitBE(8,streamFile)!=0x57415645) /* "WAVE" */
        goto fail;

    riff_size = read_32bitLE(4,streamFile);
    file_size = get_streamfile_size(streamFile);

    /* check for tructated RIFF */
    if (file_size < riff_size+8) goto fail;

    /* read through chunks to verify format and find metadata */
    {
        off_t current_chunk = 0xc; /* start with first chunk */

        while (current_chunk < file_size && current_chunk < riff_size+8) {
            uint32_t chunk_type = read_32bitBE(current_chunk,streamFile);
            off_t chunk_size = read_32bitLE(current_chunk+4,streamFile);

            if (current_chunk+8+chunk_size > file_size) goto fail;

            switch(chunk_type) {
                case 0x666d7420:    /* "fmt " */
                    /* only one per file */
                    if (FormatChunkFound) goto fail;
                    FormatChunkFound = 1;

                    sample_rate = read_32bitLE(current_chunk+0x0c,streamFile);
                    channel_count = read_16bitLE(current_chunk+0x0a,streamFile);

                    switch (read_16bitLE(current_chunk+0x8,streamFile)) {
                        case 1: /* PCM */
                            switch (read_16bitLE(current_chunk+0x16,streamFile)) {
                                case 16:
                                    coding_type = coding_PCM16LE;
                                    interleave = 2;
                                    break;
                                case 8:
                                    coding_type = coding_PCM8_U_int;
                                    interleave = 1;
                                    break;
                                default:
                                    goto fail;
                            }
                            break;
                        case 0x555: /* Level-5 0x555 ADPCM */
                            if (!mwv) goto fail;
                            coding_type = coding_L5_555;
                            interleave = 0x12;
                            break;
                        default:
                            goto fail;
                    }
                    break;
                case 0x64617461:    /* data */
                    /* at most one per file */
                    if (DataChunkFound) goto fail;
                    DataChunkFound = 1;

                    start_offset = current_chunk + 8;
                    data_size = chunk_size;
                    break;
                case 0x4C495354:    /* LIST */
                    /* what lurks within?? */
                    switch (read_32bitBE(current_chunk + 8, streamFile)) {
                        case 0x6164746C:    /* adtl */
                            /* yay, atdl is its own little world */
                            parse_adtl(current_chunk + 8, chunk_size,
                                    streamFile,
                                    &loop_start_ms,&loop_end_ms,&loop_flag);
                            break;
                        default:
                            break;
                    }
                    break;
                case 0x736D706C:    /* smpl */
                    /* check loop count */
                    if (read_32bitLE(current_chunk+0x24, streamFile)==1)
                    {
                        /* check loop info */
                        if (read_32bitLE(current_chunk+0x2c+4, streamFile)==0)
                        {
                            loop_flag = 1;
                            loop_start_offset =
                                read_32bitLE(current_chunk+0x2c+8, streamFile);
                            loop_end_offset =
                                read_32bitLE(current_chunk+0x2c+0xc,streamFile);
                        }
                    }
                    break;
                case 0x70666c74:    /* pflt */
                    if (!mwv) break;    /* ignore if not in an mwv */
                    /* predictor filters */
                    mwv_pflt_offset = current_chunk;
                    break;
                case 0x6374726c:    /* ctrl */
                    if (!mwv) break;    /* ignore if not in an mwv */
                    /* loops! */
                    if (read_32bitLE(current_chunk+8, streamFile))
                    {
                        loop_flag = 1;
                    }
                    mwv_ctrl_offset = current_chunk;
                    break;
                default:
                    /* ignorance is bliss */
                    break;
            }

            current_chunk += 8+chunk_size;
        }
    }

    if (!FormatChunkFound || !DataChunkFound) goto fail;

    switch (coding_type) {
        case coding_PCM16LE:
            sample_count = data_size/2/channel_count;
            break;
        case coding_PCM8_U_int:
            sample_count = data_size/channel_count;
            break;
        case coding_L5_555:
            sample_count = data_size/0x12/channel_count*32;
            break;
    }

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = sample_count;
    vgmstream->sample_rate = sample_rate;

    vgmstream->coding_type = coding_type;
    if (channel_count > 1 && coding_type != coding_PCM8_U_int)
        vgmstream->layout_type = layout_interleave;
    else
        vgmstream->layout_type = layout_none;
    vgmstream->interleave_block_size = interleave;

    if (loop_flag) {
        if (loop_start_ms >= 0)
        {
            vgmstream->loop_start_sample =
                (long long)loop_start_ms*sample_rate/1000;
            vgmstream->loop_end_sample =
                (long long)loop_end_ms*sample_rate/1000;
            vgmstream->meta_type = meta_RIFF_WAVE_labl_Marker;
        }
        else if (loop_start_offset >= 0)
        {
            vgmstream->loop_start_sample = loop_start_offset;
            vgmstream->loop_end_sample = loop_end_offset;
            vgmstream->meta_type = meta_RIFF_WAVE_smpl;
        }
        else if (mwv && mwv_ctrl_offset != -1)
        {
            vgmstream->loop_start_sample = read_32bitLE(mwv_ctrl_offset+12,
                    streamFile);
            vgmstream->loop_end_sample = sample_count;
        }
    }
    else
    {
        vgmstream->meta_type = meta_RIFF_WAVE;
    }

    if (mwv)
    {
        int i, c;
        if (coding_type == coding_L5_555)
        {
            const int filter_order = 3;
            int filter_count = read_32bitLE(mwv_pflt_offset+12, streamFile);

            if (mwv_pflt_offset == -1 ||
                    read_32bitLE(mwv_pflt_offset+8, streamFile) != filter_order ||
                    read_32bitLE(mwv_pflt_offset+4, streamFile) < 8 + filter_count * 4 * filter_order)
                goto fail;
            if (filter_count > 0x20) goto fail;
            for (c = 0; c < channel_count; c++)
            {
                for (i = 0; i < filter_count * filter_order; i++)
                {
                    vgmstream->ch[c].adpcm_coef_3by32[i] = read_32bitLE(
                            mwv_pflt_offset+16+i*4, streamFile
                            );
                }
            }
        }
        vgmstream->meta_type = meta_RIFF_WAVE_MWV;
    }

    /* open the file, set up each channel */
    {
        int i;

        vgmstream->ch[0].streamfile = streamFile->open(streamFile,filename,
                STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!vgmstream->ch[0].streamfile) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = vgmstream->ch[0].streamfile;
            vgmstream->ch[i].offset = vgmstream->ch[i].channel_start_offset =
                start_offset+i*interleave;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

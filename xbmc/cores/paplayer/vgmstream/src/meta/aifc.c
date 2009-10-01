#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* Audio Interchange File Format AIFF-C */
/* also plain AIFF, for good measure */

/* Included primarily for 3DO */

/* for reading integers inexplicably packed into 80 bit floats */
uint32_t read80bitSANE(off_t offset, STREAMFILE *streamFile) {
    uint8_t buf[10];
    int32_t exponent;
    int32_t mantissa;
    int i;

    if (read_streamfile(buf,offset,10,streamFile) != 10) return 0;

    exponent = ((buf[0]<<8)|(buf[1]))&0x7fff;
    exponent -= 16383;

    mantissa = 0;
    for (i=0;i<8;i++) {
        int32_t shift = exponent-7-i*8;
        if (shift >= 0)
            mantissa |= buf[i+2] << shift;
        else if (shift > -8)
            mantissa |= buf[i+2] >> -shift;
    }

    return mantissa*((buf[0]&0x80)?-1:1);
}

uint32_t find_marker(STREAMFILE *streamFile, off_t MarkerChunkOffset,
        int marker_id) {
    uint16_t marker_count;
    int i;
    off_t marker_offset;

    marker_count = read_16bitBE(MarkerChunkOffset+8,streamFile);
    marker_offset = MarkerChunkOffset+10;
    for (i=0;i<marker_count;i++) {
        int name_length;
        
        if (read_16bitBE(marker_offset,streamFile) == marker_id)
            return read_32bitBE(marker_offset+2,streamFile);

        name_length = (uint8_t)read_8bit(marker_offset+6,streamFile) + 1;
        if (name_length % 2) name_length++;
        marker_offset += 6 + name_length;
    }

    return -1;
}

VGMSTREAM * init_vgmstream_aifc(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    off_t file_size = -1;
    int channel_count = 0;
    int sample_count = 0;
    int sample_size = 0;
    int sample_rate = 0;
    int coding_type = -1;
    off_t start_offset = -1;
    int interleave = -1;

    int loop_flag = 0;
    int32_t loop_start = -1;
    int32_t loop_end = -1;

    int AIFFext = 0;
    int AIFCext = 0;
    int AIFF = 0;
    int AIFC = 0;
    int FormatVersionChunkFound = 0;
    int CommonChunkFound = 0;
    int SoundDataChunkFound = 0;
    int MarkerChunkFound = 0;
    off_t MarkerChunkOffset = -1;
    int InstrumentChunkFound =0;
    off_t InstrumentChunkOffset = -1;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (!strcasecmp("aifc",filename_extension(filename)) ||
        !strcasecmp("afc",filename_extension(filename)) ||
        !strcasecmp("aifcl",filename_extension(filename)))
    {
        AIFCext = 1;
    }
    else if (!strcasecmp("aiff",filename_extension(filename)) ||
        !strcasecmp("aif",filename_extension(filename)) ||
        !strcasecmp("aiffl",filename_extension(filename)))
    {
        AIFFext = 1;
    }
    else goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)==0x464F524D &&  /* "FORM" */
        /* check that file = header (8) + data */
        (uint32_t)read_32bitBE(4,streamFile)+8==get_streamfile_size(streamFile))
    {
        if ((uint32_t)read_32bitBE(8,streamFile)==0x41494643) /* "AIFC" */
        {
            if (!AIFCext) goto fail;
            AIFC = 1;
        }
        else if ((uint32_t)read_32bitBE(8,streamFile)==0x41494646) /* "AIFF" */
        {
            if (!AIFFext) goto fail;
            AIFF = 1;
        }
        else goto fail;
    } else goto fail;
    
    file_size = get_streamfile_size(streamFile);

    /* read through chunks to verify format and find metadata */
    {
        off_t current_chunk = 0xc; /* start with first chunk within FORM */

        while (current_chunk < file_size) {
            uint32_t chunk_type = read_32bitBE(current_chunk,streamFile);
            off_t chunk_size = read_32bitBE(current_chunk+4,streamFile);

            /* chunks must be padded to an even number of bytes but chunk
             * size does not include that padding */
            if (chunk_size % 2) chunk_size++;

            if (current_chunk+8+chunk_size > file_size) goto fail;

            switch(chunk_type) {
                case 0x46564552:    /* FVER */
                    /* only one per file */
                    if (FormatVersionChunkFound) goto fail;
                    /* plain AIFF shouldn't have */
                    if (AIFF) goto fail;
                    FormatVersionChunkFound = 1;

                    /* specific size */
                    if (chunk_size != 4) goto fail;

                    /* Version 1 of AIFF-C spec timestamp */
                    if ((uint32_t)read_32bitBE(current_chunk+8,streamFile) !=
                            0xA2805140) goto fail;
                    break;
                case 0x434F4D4D:    /* COMM */
                    /* only one per file */
                    if (CommonChunkFound) goto fail;
                    CommonChunkFound = 1;

                    channel_count = read_16bitBE(current_chunk+8,streamFile);
                    if (channel_count <= 0) goto fail;

                    sample_count = (uint32_t)read_32bitBE(current_chunk+0xa,streamFile);

                    sample_size = read_16bitBE(current_chunk+0xe,streamFile);

                    sample_rate = read80bitSANE(current_chunk+0x10,streamFile);

                    if (AIFC) {
                        switch (read_32bitBE(current_chunk+0x1a,streamFile)) {
                            case 0x53445832:    /* SDX2 */
                                coding_type = coding_SDX2;
                                interleave = 1;
                                break;
                            case 0x41445034:    /* ADP4 */
                                coding_type = coding_DVI_IMA;
                                /* don't know how stereo DVI is laid out */
                                if (channel_count != 1) break;
                                break;
                            default:
                                /* we should probably support uncompressed here */
                                goto fail;
                        }
                    } else if (AIFF) {
                        switch (sample_size) {
                            case 8:
                                coding_type = coding_PCM8;
                                interleave = 1;
                                break;
                            case 16:
                                coding_type = coding_PCM16BE;
                                interleave = 2;
                                break;
                            /* 32 is a possibility, but we don't see it and I
                             * don't have a reader for it yet */
                            default:
                                goto fail;
                        }
                    }
                    
                    /* we don't check the human-readable portion of AIFF-C*/

                    break;
                case 0x53534E44:    /* SSND */
                    /* at most one per file */
                    if (SoundDataChunkFound) goto fail;
                    SoundDataChunkFound = 1;

                    start_offset = current_chunk + 16 + read_32bitBE(current_chunk+8,streamFile);
                    break;
                case 0x4D41524B:    /* MARK */
                    if (MarkerChunkFound) goto fail;
                    MarkerChunkFound = 1;
                    MarkerChunkOffset = current_chunk;
                    break;
                case 0x494E5354:    /* INST */
                    if (InstrumentChunkFound) goto fail;
                    InstrumentChunkFound = 1;
                    InstrumentChunkOffset = current_chunk;
                    break;
                default:
                    /* spec says we can skip unrecognized chunks */
                    break;
            }

            current_chunk += 8+chunk_size;
        }
    }

    if (AIFC) {
        if (!FormatVersionChunkFound || !CommonChunkFound || !SoundDataChunkFound)
            goto fail;
    } else if (AIFF) {
        if (!CommonChunkFound || !SoundDataChunkFound)
            goto fail;
    }

    /* read loop points */
    if (InstrumentChunkFound && MarkerChunkFound) {
        int start_marker;
        int end_marker;
        /* use the sustain loop */
        /* if playMode=ForwardLooping */
        if (read_16bitBE(InstrumentChunkOffset+16,streamFile) == 1) {
            start_marker = read_16bitBE(InstrumentChunkOffset+18,streamFile);
            end_marker = read_16bitBE(InstrumentChunkOffset+20,streamFile);
            /* check for sustain markers != 0 (invalid marker no) */
            if (start_marker && end_marker) {
                /* find start marker */
                loop_start = find_marker(streamFile,MarkerChunkOffset,start_marker);
                loop_end = find_marker(streamFile,MarkerChunkOffset,end_marker);

                /* find_marker is type uint32_t as the spec says that's the type
                 * of the position value, but it returns a -1 on error, and the
                 * loop_start and loop_end variables are int32_t, so the error
                 * will become apparent.
                 * We shouldn't have a loop point that overflows an int32_t
                 * anyway. */
                loop_flag = 1;
                if (loop_start==loop_end) loop_flag = 0;
            }
        }
    }

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = sample_count;
    vgmstream->sample_rate = sample_rate;

    vgmstream->coding_type = coding_type;
    if (channel_count > 1)
        vgmstream->layout_type = layout_interleave;
    else
        vgmstream->layout_type = layout_none;
    vgmstream->interleave_block_size = interleave;
    vgmstream->loop_start_sample = loop_start;
    vgmstream->loop_end_sample = loop_end;

    if (AIFC)
        vgmstream->meta_type = meta_AIFC;
    else if (AIFF)
        vgmstream->meta_type = meta_AIFF;

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

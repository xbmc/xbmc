#include "../vgmstream.h"

#ifdef VGM_USE_VORBIS

#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* .sfl, odd RIFF-formatted files that go along with oggs */

/* return milliseconds */
static int parse_region(unsigned char * region, long *start, long *end) {
    long start_hh,start_mm,start_ss,start_ms;
    long end_hh,end_mm,end_ss,end_ms;

    if (memcmp("Region ",region,7)) return -1;

    if (8 != sscanf((char*)region+7,"%ld:%ld:%ld.%ld to %ld:%ld:%ld.%ld",
                &start_hh,&start_mm,&start_ss,&start_ms,
                &end_hh,&end_mm,&end_ss,&end_ms))
        return -1;

    *start = ((start_hh*60+start_mm)*60+start_ss)*1000+start_ms;
    *end = ((end_hh*60+end_mm)*60+end_ss)*1000+end_ms;

    return 0;
}

/* loop points have been found hiding here */
static void parse_adtl(off_t adtl_offset, off_t adtl_length, STREAMFILE  *streamFile,
        long *loop_start, long *loop_end, int *loop_flag) {
    int loop_found = 0;

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

                    if (!loop_found &&
                                parse_region(labelcontent,loop_start,loop_end)>=0)
                    {
                        loop_found = 1;
                    }

                    free(labelcontent);
                }
                break;
            default:
                break;
        }

        current_chunk += 8 + chunk_size;
    }

    if (loop_found) *loop_flag = 1;
}

VGMSTREAM * init_vgmstream_sfl(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileOGG = NULL;
    char filenameOGG[260];
    char filename[260];

    off_t file_size = -1;

    int loop_flag = 0;
    long loop_start_ms = -1;
    long loop_end_ms = -1;
    uint32_t riff_size;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sfl",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x52494646) /* "RIFF" */
        goto fail;
    /* check for SFPL form */
    if ((uint32_t)read_32bitBE(8,streamFile)!=0x5346504C) /* "SFPL" */
        goto fail;

   /* check for .OGG file */
    strcpy(filenameOGG,filename);
    strcpy(filenameOGG+strlen(filenameOGG)-3,"ogg");

    streamFileOGG = streamFile->open(streamFile,filenameOGG,STREAMFILE_DEFAULT_BUFFER_SIZE);
    if (!streamFileOGG) {
        goto fail;
    }

    /* let the real initer do the parsing */
    vgmstream = init_vgmstream_ogg_vorbis(streamFileOGG);
    if (!vgmstream) goto fail;

    close_streamfile(streamFileOGG);
    streamFileOGG = NULL;

    /* now that we have an ogg, proceed with parsing the .sfl */
    riff_size = read_32bitLE(4,streamFile);
    file_size = get_streamfile_size(streamFile);

    /* check for tructated RIFF */
    if (file_size < riff_size+8) goto fail;

    /* read through chunks to verify format and find metadata */
    {
        off_t current_chunk = 0xc; /* start with first chunk */

        while (current_chunk < file_size) {
            uint32_t chunk_type = read_32bitBE(current_chunk,streamFile);
            off_t chunk_size = read_32bitLE(current_chunk+4,streamFile);

            /* There seem to be a few bytes left over, included in the
             * RIFF but not enough to make a new chunk.
             */
            if (current_chunk+8 > file_size) break;

            if (current_chunk+8+chunk_size > file_size) goto fail;

            switch(chunk_type) {
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
                default:
                    /* ignorance is bliss */
                    break;
            }

            current_chunk += 8+chunk_size;
        }
    }

    if (loop_flag) {
        /* install loops */
        if (!vgmstream->loop_flag) {
            vgmstream->loop_flag = 1;
            vgmstream->loop_ch = calloc(vgmstream->channels,
                    sizeof(VGMSTREAMCHANNEL));
            if (!vgmstream->loop_ch) goto fail;
        }

        vgmstream->loop_start_sample = (long long)loop_start_ms*vgmstream->sample_rate/1000;
        vgmstream->loop_end_sample = (long long)loop_end_ms*vgmstream->sample_rate/1000;
    }

    vgmstream->meta_type = meta_OGG_SFL;

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileOGG) close_streamfile(streamFileOGG);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

#endif

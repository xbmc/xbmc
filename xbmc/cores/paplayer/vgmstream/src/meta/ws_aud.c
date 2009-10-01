#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* Westwood Studios .aud (WS-AUD) */

VGMSTREAM * init_vgmstream_ws_aud(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    coding_t coding_type = -1;
    off_t format_offset;

    int channel_count;
    int new_type = 0;   /* if 0 is old type */

    int bytes_per_sample = 0;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("aud",filename_extension(filename))) goto fail;

    /* check for 0x0000DEAF chunk marker for first chunk */
    if (read_32bitLE(0x10,streamFile)==0x0000DEAF) {    /* new */
        new_type = 1;
    } else if (read_32bitLE(0x0C,streamFile)==0x0000DEAF) { /* old */
        new_type = 0;
    } else goto fail;

    if (new_type)
        format_offset = 0xa;
    else
        format_offset = 0x6;

    /* get channel count */
    if (read_8bit(format_offset,streamFile) & 1)
        channel_count = 2;
    else
        channel_count = 1;

    if (channel_count == 2) goto fail; /* TODO: not yet supported (largely
                                          because not yet seen) */

    /* get output format */
    if (read_8bit(format_offset+1,streamFile) & 2)
        bytes_per_sample = 2;
    else
        bytes_per_sample = 1;

    /* check codec type */
    switch (read_8bit(format_offset+1,streamFile)) {
        case 1:     /* Westwood custom */
            coding_type = coding_WS;
            /* shouldn't happen? */
            if (bytes_per_sample != 1) goto fail;
            break;
        case 99:    /* IMA ADPCM */
            coding_type = coding_IMA;
            break;
        default:
            goto fail;
            break;
    }

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(channel_count,0);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    if (new_type) {
        vgmstream->num_samples = read_32bitLE(0x06,streamFile)/bytes_per_sample/channel_count;
    } else {
        /* Doh, no output size in old type files. We have to read through the
         * file looking at chunk headers! Crap! */
        int32_t out_size = 0;
        off_t current_offset = 0x8;
        off_t file_size = get_streamfile_size(streamFile);

        while (current_offset < file_size) {
            int16_t chunk_size;
            chunk_size = read_16bitLE(current_offset,streamFile);
            out_size += read_16bitLE(current_offset+2,streamFile);
            /* while we're here might as well check for valid chunks */
            if (read_32bitLE(current_offset+4,streamFile) != 0x0000DEAF) goto fail;
            current_offset+=8+chunk_size;
        }

        vgmstream->num_samples = out_size/bytes_per_sample/channel_count;
    }
    
    /* they tend to not actually have data for the last odd sample */
    if (vgmstream->num_samples & 1) vgmstream->num_samples--;
    vgmstream->sample_rate = (uint16_t)read_16bitLE(0x00,streamFile);

    vgmstream->coding_type = coding_type;
    if (new_type) {
        vgmstream->meta_type = meta_WS_AUD;
    } else {
        vgmstream->meta_type = meta_WS_AUD_old;
    }

    vgmstream->layout_type = layout_ws_aud_blocked;

    /* open the file for reading by each channel */
    {
        int i;
        STREAMFILE * file;

        file = streamFile->open(streamFile,filename,
                STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;

        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;
        }
    }

    /* start me up */
    if (new_type) {
        ws_aud_block_update(0xc,vgmstream);
    } else {
        ws_aud_block_update(0x8,vgmstream);
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

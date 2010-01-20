#include "meta.h"
#include "../util.h"

/* XSS (found in Dino Crisis 3) */
VGMSTREAM * init_vgmstream_xss(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
    int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("xss",filename_extension(filename))) goto fail;

    /* check header */
    if ((uint16_t)read_16bitLE(0x15A,streamFile) != 0x10)
        goto fail;
    if (read_32bitLE(0x154,streamFile) / read_32bitLE(0x150,streamFile) !=
        (uint16_t)read_16bitLE(0x158,streamFile))
        goto fail;

    loop_flag = (read_32bitLE(0x144,streamFile)!=0);
    channel_count = (uint16_t)read_16bitLE(0x14E,streamFile);
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    start_offset = 0x800;
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x150,streamFile);
    vgmstream->coding_type = coding_PCM16LE;
    vgmstream->num_samples = (get_streamfile_size(streamFile)-start_offset)/2/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x144,streamFile)/2/channel_count;
        vgmstream->loop_end_sample = read_32bitLE(0x148,streamFile)/2/channel_count;
    }

    if (vgmstream->channels == 1) {
        vgmstream->layout_type = layout_none;
    } else if (vgmstream->channels > 1) {
        vgmstream->layout_type = layout_interleave;
        vgmstream->interleave_block_size = 0x2;
    }

    vgmstream->meta_type = meta_XSS;

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+
                vgmstream->interleave_block_size*i;

        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

#include "meta.h"
#include "../util.h"

/* IKM (found in Zwei!) */
VGMSTREAM * init_vgmstream_ikm(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
    int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("ikm",filename_extension(filename))) goto fail;

    /* check header */
    if (read_32bitBE(0x00,streamFile) != 0x494B4D00)    /* "IKM\0" */
        goto fail;
    if (read_32bitBE(0x40,streamFile) != 0x41535400)    /* AST\0 */
        goto fail;

    loop_flag = (read_32bitLE(0x14,streamFile)!=0); /* Not sure */
    channel_count = read_32bitLE(0x50,streamFile);
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    start_offset = 0x800;
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x44,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = (read_32bitLE(0x4C,streamFile)-start_offset)/16/channel_count*28;
    if (loop_flag) {
        vgmstream->loop_start_sample = read_32bitLE(0x14,streamFile);
        vgmstream->loop_end_sample = read_32bitLE(0x18,streamFile);
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x10;
    vgmstream->meta_type = meta_IKM;

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

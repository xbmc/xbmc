#include "meta.h"
#include "../util.h"

/* LEG - found in Legaia 2 - Duel Saga (PS2)
the headers are stored seperately in the main executable... */
VGMSTREAM * init_vgmstream_leg(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
    int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("leg",filename_extension(filename))) goto fail;

    /* comparing the filesize with (num_samples*0x800) + headersize,
    if it doesn't match, we will abort the vgmstream... */
    if ((read_32bitLE(0x48,streamFile)*0x800)+0x4C != get_streamfile_size(streamFile))
        goto fail;

    loop_flag = (read_32bitLE(0x44,streamFile)!=0);
    channel_count = 2;
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    start_offset = 0x4C;
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x40,streamFile);
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = (read_32bitLE(0x48,streamFile)*0x800)*28/16/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitLE(0x44,streamFile)*0x800)*28/16/channel_count;
        vgmstream->loop_end_sample = (read_32bitLE(0x48,streamFile)*0x800)*28/16/channel_count;
    }

    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x400;
    vgmstream->meta_type = meta_LEG;

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

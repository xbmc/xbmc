#include "meta.h"
#include "../util.h"

/* ASD - found in Miss Moonlight (DC) */
VGMSTREAM * init_vgmstream_dc_asd(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag;
    int channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("asd",filename_extension(filename))) goto fail;

    /* We have no "Magic" words in this header format, so we have to do some,
    other checks, it seems the samplecount is stored twice in the header,
    we'll compare it... */
    if (read_32bitLE(0x0,streamFile) != read_32bitLE(0x4,streamFile))
        goto fail;
    /* compare the frequency with the bitrate, if it doesn't match we'll close 
    the vgmstream... */
    if (read_32bitLE(0x10,streamFile)/read_32bitLE(0xC,streamFile) != (uint16_t)read_16bitLE(0xA,streamFile)*2)
        goto fail;

    loop_flag = 0;
    channel_count = read_16bitLE(0x0A,streamFile);
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    start_offset = get_streamfile_size(streamFile) - read_32bitLE(0x0,streamFile);
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x0C,streamFile);
    vgmstream->coding_type = coding_PCM16LE;
    vgmstream->num_samples = read_32bitLE(0x0,streamFile)/2/channel_count;
    if (loop_flag) {
        vgmstream->loop_start_sample = 0;
        vgmstream->loop_end_sample = read_32bitLE(0x0,streamFile)/2/channel_count;
    }
    
    vgmstream->meta_type = meta_DC_ASD;

    if (vgmstream->channels == 1) {
        vgmstream->layout_type = layout_none;
    } else if (vgmstream->channels == 2) {
        vgmstream->layout_type = layout_interleave;
        vgmstream->interleave_block_size = 0x2;
    }

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

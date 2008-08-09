#include "meta.h"
#include "../util.h"
#include "../coding/coding.h"

/* .rsf - from Metroid Prime */

VGMSTREAM * init_vgmstream_rsf(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    size_t file_size;

    /* check extension, case insensitive */
    /* this is all we have to go on, rsf is completely headerless */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("rsf",filename_extension(filename))) goto fail;

    file_size = get_streamfile_size(streamFile);

    {
        /* extra check: G.721 has no zero nibbles, so we look at
         * the first few bytes*/
        int8_t test_byte;
        off_t i;
        /* 0x20 is arbitrary, all files are much larger */
        for (i=0;i<0x20;i++) {
            test_byte = read_8bit(i,streamFile);
            if (!(test_byte&0xf) || !(test_byte&0xf0)) goto fail;
        }
        /* and also check start of second channel */
        for (i=(file_size+1)/2;i<(file_size+1)/2+0x20;i++) {
            test_byte = read_8bit(i,streamFile);
            if (!(test_byte&0xf) || !(test_byte&0xf0)) goto fail;
        }
    }

    /* build the VGMSTREAM */

    vgmstream = allocate_vgmstream(2,0);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->num_samples = file_size;
    vgmstream->sample_rate = 32000;

    vgmstream->coding_type = coding_G721;
    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_RSF;

    /* open the file for reading by each channel */
    {
        int i;
        for (i=0;i<2;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (file_size+1)/2*i;


            g72x_init_state(&(vgmstream->ch[i].g72x_state));
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

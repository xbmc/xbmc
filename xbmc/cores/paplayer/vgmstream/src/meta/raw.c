#include "meta.h"
#include "../util.h"

/* RAW

   RAW format is native 44khz PCM file
   Nothing more :P ...

   2008-05-17 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_raw(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
	int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("raw",filename_extension(filename))) goto fail;

    /* No check to do as they are raw pcm */

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(2,0);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = 2;
    vgmstream->sample_rate = 44100;
    vgmstream->coding_type = coding_PCM16LE;
    vgmstream->num_samples = (int32_t)(get_streamfile_size(streamFile)/4);
    vgmstream->layout_type = layout_interleave;
	vgmstream->interleave_block_size = 2;
    vgmstream->meta_type = meta_RAW;

    /* open the file for reading by each channel */
    {
        STREAMFILE *chstreamfile;

        /* have both channels use the same buffer, as interleave is so small */
        chstreamfile = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        
        if (!chstreamfile) goto fail;

        for (i=0;i<2;i++) {
            vgmstream->ch[i].streamfile = chstreamfile;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=(off_t)(i*vgmstream->interleave_block_size);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

#include "meta.h"
#include "../util.h"

/* INT

   PS2 INT format is a RAW 48khz PCM file without header                
   The only fact about those file, is that the raw is interleaved 

   The interleave value is allways 0x200
   known extensions : INT

   2008-05-11 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_ps2_int(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
	int i,channel_count;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("int",filename_extension(filename)) && 
		strcasecmp("wp2",filename_extension(filename))) goto fail;

	if(!strcasecmp("int",filename_extension(filename)))
		channel_count = 2;
	else
		channel_count = 4;

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,0);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
	vgmstream->channels=channel_count;
    vgmstream->sample_rate = 48000;
    vgmstream->coding_type = coding_PCM16LE;
    vgmstream->num_samples = (int32_t)(get_streamfile_size(streamFile)/(vgmstream->channels*2));
    vgmstream->interleave_block_size = 0x200;
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_RAW;

    /* open the file for reading by each channel */
    {
        for (i=0;i<vgmstream->channels;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=0;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

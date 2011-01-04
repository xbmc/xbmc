#include "meta.h"
#include "../util.h"

/* GMS

   PSX GMS format has no recognition ID.
   This format was used essentially in Grandia Games but 
   can be easily used by other header format as the format of the header is very simple

   known extensions : GMS 

   2008-05-19 - Fastelbja : First version ...
*/

VGMSTREAM * init_vgmstream_psx_gms(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
	int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("gms",filename_extension(filename))) goto fail;

    /* check loop */
	loop_flag = (read_32bitLE(0x20,streamFile)==0);
    
	/* Always stereo files */
	channel_count=read_32bitLE(0x00,streamFile);
    
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitLE(0x04,streamFile);

	vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x1C,streamFile);

	/* Get loop point values */
	if(vgmstream->loop_flag) {
		vgmstream->loop_start_sample = read_32bitLE(0x14,streamFile);
		vgmstream->loop_end_sample = read_32bitLE(0x1C,streamFile);
	}

    vgmstream->layout_type = layout_interleave;
	vgmstream->interleave_block_size = 0x800;
    vgmstream->meta_type = meta_PSX_GMS;

	start_offset = 0x800;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,vgmstream->interleave_block_size);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=
                (off_t)(start_offset+vgmstream->interleave_block_size*i);
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

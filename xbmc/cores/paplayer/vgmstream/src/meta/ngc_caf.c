#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

VGMSTREAM * init_vgmstream_caf(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    
	// Calculate sample length ...
	int32_t	num_of_samples=0;
	int32_t	block_count=0;

	uint32_t loop_start=-1;

	off_t	offset=0;
	off_t	next_block;
	off_t	file_length;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("cfn",filename_extension(filename))) goto fail;

    /* Check "CAF " ID */
    if (read_32bitBE(0,streamFile)!=0x43414620) goto fail;

	// Calculate sample length ...
	file_length=(off_t)get_streamfile_size(streamFile);

	do {
		next_block=read_32bitBE(offset+0x04,streamFile);
		num_of_samples+=read_32bitBE(offset+0x14,streamFile)/8*14;

		if(read_32bitBE(offset+0x20,streamFile)==read_32bitBE(offset+0x08,streamFile)) {
			loop_start=num_of_samples-read_32bitBE(offset+0x14,streamFile)/8*14;
		}
		offset+=next_block;
		block_count++;
	}  while(offset<file_length);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(2,(loop_start!=-1));    /* always stereo */
    if (!vgmstream) goto fail;

	vgmstream->channels=2;
	vgmstream->sample_rate=32000;
	vgmstream->num_samples=num_of_samples;

	if(loop_start!=-1) {
		vgmstream->loop_start_sample=loop_start;
		vgmstream->loop_end_sample=num_of_samples;
	}

    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->layout_type = layout_caf_blocked;
    vgmstream->meta_type = meta_CFN;

    /* open the file for reading by each channel */
    {
        for (i=0;i<2;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

	caf_block_update(0,vgmstream);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

#include "meta.h"
#include "../util.h"

/* Sony .ADS with SShd & SSbd Headers */

VGMSTREAM * init_vgmstream_ps2_ads(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];

    int loop_flag=0;
    int channel_count;
	off_t start_offset,test_offset;
	int ads_type_2=0;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("ads",filename_extension(filename)) && 
        strcasecmp("ss2",filename_extension(filename))) goto fail;

    /* check SShd Header */
    if (read_32bitBE(0x00,streamFile) != 0x53536864)
        goto fail;

    /* check SSbd Header */
    if (read_32bitBE(0x20,streamFile) != 0x53536264)
        goto fail;

    /* check if file is not corrupt */
    if (get_streamfile_size(streamFile)<(size_t)(read_32bitLE(0x24,streamFile) + 0x28))
        goto fail;

    /* check loop */
    loop_flag = (read_32bitLE(0x1C,streamFile)!=0xFFFFFFFF);

    channel_count=read_32bitLE(0x10,streamFile);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = read_32bitLE(0x10,streamFile);
    vgmstream->sample_rate = read_32bitLE(0x0C,streamFile);

    /* Check for Compression Scheme */
    vgmstream->coding_type = coding_PSX;
    vgmstream->num_samples = read_32bitLE(0x24,streamFile)/16*28/vgmstream->channels;

	/* SS2 container with RAW Interleaved PCM */
    if (read_32bitLE(0x08,streamFile)!=0x10) {
        vgmstream->coding_type=coding_PCM16LE;
        vgmstream->num_samples = read_32bitLE(0x24,streamFile)/2/vgmstream->channels;
    }

    vgmstream->interleave_block_size = read_32bitLE(0x14,streamFile);
    vgmstream->layout_type = layout_interleave;
    vgmstream->meta_type = meta_PS2_SShd;

    /* Get loop point values */
    if(vgmstream->loop_flag) {
		if(vgmstream->interleave_block_size==0x10) {
			vgmstream->loop_start_sample = read_32bitLE(0x18,streamFile);
			vgmstream->loop_end_sample = read_32bitLE(0x1C,streamFile);
		} else {
			vgmstream->loop_start_sample = (read_32bitLE(0x18,streamFile)*0x10)/16*28;
			vgmstream->loop_end_sample = (read_32bitLE(0x1C,streamFile)*0x10)/16*28;
		}
    }

    /* don't know why, but it does happen, in ps2 too :( */
    if (vgmstream->loop_end_sample > vgmstream->num_samples)
        vgmstream->loop_end_sample = vgmstream->num_samples;

	start_offset=0x28;

	// Hack for files with start_offset = 0x800
	ads_type_2=1;

	for(test_offset=start_offset;test_offset<0x800;test_offset+=4) {
		if (read_32bitLE(test_offset,streamFile)!=0)
			ads_type_2=0;
	}

	if((ads_type_2==1)){
		start_offset=0x800;
	}

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

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

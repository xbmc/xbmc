#include "meta.h"
#include "../coding/coding.h"
#include "../layout/layout.h"
#include "../util.h"

typedef struct 
{
 char	szID[4];
 int	dwSampleRate;
 char	bBits;
 char	bChannels;
 char	bCompression;
 char	bType;
 int	dwNumSamples;
 int	dwLoopStart;
 int	dwLoopLength;
 int	dwDataStart;
 int	dwUnknown;
} EACSHeader;

VGMSTREAM * init_vgmstream_eacs(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    int channel_count;
    int loop_flag;
	char little_endian=0;
	off_t	start_offset;
	EACSHeader	*ea_header = NULL;
	int32_t samples_count=0;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("cnk",filename_extension(filename)) && 
		strcasecmp("as4",filename_extension(filename)) && 
		strcasecmp("asf",filename_extension(filename))) goto fail;

	ea_header=(EACSHeader *)malloc(sizeof(EACSHeader));

    /* check header */
    if ((uint32_t)read_32bitBE(0,streamFile)!=0x31534E68) /* "1SNh" */
        goto fail;

	/* check if we are little or big endian */
    if ((uint32_t)read_32bitBE(4,streamFile)<0x40) 
        little_endian=1;

    /* check type details */
	start_offset = read_32bitLE(0x04,streamFile);

	if((uint32_t)read_32bitBE(0x08,streamFile)==0x45414353) { /* EACS */ 
		read_streamfile((uint8_t*)ea_header,0x08,sizeof(EACSHeader),streamFile);
		loop_flag = 0; //(ea_header->dwLoopStart!=0);
		channel_count = (ea_header->bChannels);
		/* build the VGMSTREAM */

		vgmstream = allocate_vgmstream(channel_count,0);
		if (!vgmstream) goto fail;

		/* fill in the vital statistics */
		init_get_high_nibble(vgmstream);
		
		vgmstream->sample_rate = ea_header->dwSampleRate;
		
		if(ea_header->bCompression==0) {
			vgmstream->coding_type = coding_PCM16LE_int;
			if(ea_header->bBits==1)
				vgmstream->coding_type = coding_PCM8_int;
		}
		else 
			vgmstream->coding_type = coding_EACS_IMA;
		
		vgmstream->layout_type = layout_eacs_blocked;
		vgmstream->meta_type = meta_EACS_PC;

		if(little_endian)
			vgmstream->meta_type = meta_EACS_SAT;

	} else {
		channel_count=read_32bitLE(0x20,streamFile);
		
		vgmstream = allocate_vgmstream(channel_count,0);
		if (!vgmstream) goto fail;

		vgmstream->sample_rate = read_32bitLE(0x08,streamFile);
		vgmstream->coding_type = coding_PSX;
		vgmstream->layout_type=layout_eacs_blocked;
		vgmstream->meta_type=meta_EACS_PSX;
	}

	vgmstream->ea_platform=little_endian;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);
            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

	// calc the samples length ...
	if(little_endian)
		vgmstream->next_block_offset=read_32bitBE(0x04,streamFile);
	else
		vgmstream->next_block_offset=read_32bitLE(0x04,streamFile);

	if(vgmstream->next_block_offset>0x30) {
		vgmstream->current_block_size=vgmstream->next_block_offset-sizeof(EACSHeader);
		samples_count=(int32_t)vgmstream->current_block_size/get_vgmstream_frame_size(vgmstream)*get_vgmstream_samples_per_frame(vgmstream);
		samples_count/=vgmstream->channels;
	}
	
	do {
		if(read_32bitBE(vgmstream->next_block_offset,vgmstream->ch[0].streamfile)==0x31534E6C) {
			ea_header->dwLoopStart=read_32bitLE(vgmstream->next_block_offset+0x08,vgmstream->ch[0].streamfile);
			vgmstream->next_block_offset+=0x0C;
		}

		if(read_32bitBE(vgmstream->next_block_offset,vgmstream->ch[0].streamfile)==0x31534E65) 
			break;

		eacs_block_update(vgmstream->next_block_offset,vgmstream);
		samples_count+=vgmstream->current_block_size/get_vgmstream_frame_size(vgmstream)*get_vgmstream_samples_per_frame(vgmstream);
	} while(vgmstream->next_block_offset<get_streamfile_size(streamFile)-8);

	// Reset values ...
	// setting up the first header by calling the eacs_block_update sub
	if(little_endian)
		vgmstream->next_block_offset=read_32bitBE(0x04,streamFile);
	else
		vgmstream->next_block_offset=read_32bitLE(0x04,streamFile);

	vgmstream->current_block_size=vgmstream->next_block_offset-sizeof(EACSHeader);
	
	if(vgmstream->coding_type!=coding_PSX)
		vgmstream->current_block_size-=8;

	if(vgmstream->coding_type==coding_PSX) 
		eacs_block_update(0x2C,vgmstream);
	else 
		eacs_block_update(0x28,vgmstream);

	// re-allocate the sample count
	vgmstream->num_samples=samples_count;
	
	if(loop_flag) {
		vgmstream->loop_start_sample = ea_header->dwLoopStart;
		vgmstream->loop_end_sample = vgmstream->num_samples;
	}

	if(ea_header) 
		free(ea_header);

    return vgmstream;

    /* clean up anything we may have opened */
fail:
	if(ea_header) 
		free(ea_header);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

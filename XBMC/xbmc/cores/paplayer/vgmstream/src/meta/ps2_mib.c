#include "meta.h"
#include "../util.h"

/* MIB

   PS2 MIB format is a headerless format.
   The interleave value can be found by checking the body of the data.
   
   The interleave start allways at offset 0 with a int value (which can have
   many values : 0x0000, 0x0002, 0x0006 etc...) follow by 12 empty (zero) values.

   The interleave value is the offset where you found the same 16 bytes.

   The n° of channels can be found by checking each time you found this 16 bytes.

   The interleave value can be very "large" (up to 0x20000 found so far) and is allways
   a 0x10 multiply value.
   
   The loop values can be found by checking the 'tags' offset (found @ 0x02 each 0x10 bytes).
   06 = start of the loop point (can be found for each channel)
   03 - end of the loop point (can be found for each channel)

   The .MIH header contains all informations about frequency, numbers of channels, interleave
   but has, afaik, no loop values.

   known extensions : MIB (MIH for the header) MIC (concatenation of MIB+MIH)
					  Nota : the MIC stuff is not supported here as there is
							 another MIC format which can be found in Koei Games.

   2008-05-14 - Fastelbja : First version ...
   2008-05-20 - Fastelbja : Fix loop value when loopEnd==0
*/

VGMSTREAM * init_vgmstream_ps2_mib(STREAMFILE *streamFile) {
    
	VGMSTREAM * vgmstream = NULL;
    STREAMFILE * streamFileMIH = NULL;
    char filename[260];
    
	uint8_t mibBuffer[0x10];
	uint8_t	testBuffer[0x10];

	size_t	fileLength;
	
	off_t	loopStart = 0;
	off_t	loopEnd = 0;
	off_t	interleave = 0;
	off_t	readOffset = 0;

	char   filenameMIH[260];

	uint8_t gotMIH=0;

	int i, channel_count=1;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("mib",filename_extension(filename)) && 
		strcasecmp("mi4",filename_extension(filename))) goto fail;

	/* check for .MIH file */
	strcpy(filenameMIH,filename);
	strcpy(filenameMIH+strlen(filenameMIH)-3,"MIH");

	streamFileMIH = streamFile->open(streamFile,filenameMIH,STREAMFILE_DEFAULT_BUFFER_SIZE);
	if (streamFileMIH) gotMIH = 1;

    /* Search for interleave value & loop points */
	/* Get the first 16 values */
	fileLength = get_streamfile_size(streamFile);
	
	readOffset+=(off_t)read_streamfile(mibBuffer,0,0x10,streamFile); 

	do {
		readOffset+=(off_t)read_streamfile(testBuffer,readOffset,0x10,streamFile); 
		
		if(!memcmp(testBuffer,mibBuffer,0x10)) {
			if(interleave==0) interleave=readOffset-0x10;

			// be sure to point to an interleave value
			if(((readOffset-0x10)==channel_count*interleave)) {
				channel_count++;
			}
		}

		// Loop Start ...
		if(testBuffer[0x01]==0x06) {
			if(loopStart==0) loopStart = readOffset-0x10;
		}

		// Loop End ...
		if(testBuffer[0x01]==0x03) {
			if(loopEnd==0) loopEnd = readOffset-0x10;
		}

	} while (streamFile->get_offset(streamFile)<(int32_t)fileLength);

	if(gotMIH) 
		channel_count=read_32bitLE(0x08,streamFileMIH);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,((loopStart!=0) && (loopEnd!=0)));
    if (!vgmstream) goto fail;

	if(interleave==0) interleave=0x10;

    /* fill in the vital statistics */
	if(gotMIH) {
		// Read stuff from the MIH file 
		vgmstream->channels = read_32bitLE(0x08,streamFileMIH);
		vgmstream->sample_rate = read_32bitLE(0x0C,streamFileMIH);
		vgmstream->interleave_block_size = read_32bitLE(0x10,streamFileMIH);
		vgmstream->num_samples=((read_32bitLE(0x10,streamFileMIH)*
								(read_32bitLE(0x14,streamFileMIH)-1)*2)+
								((read_32bitLE(0x04,streamFileMIH)>>8)*2))/16*28/2;
	} else {
		vgmstream->channels = channel_count;
		vgmstream->interleave_block_size = interleave;

		if(!strcasecmp("mib",filename_extension(filename)))
			vgmstream->sample_rate = 44100;

		if(!strcasecmp("mi4",filename_extension(filename)))
			vgmstream->sample_rate = 48000;

		vgmstream->num_samples = (int32_t)(fileLength/16/channel_count*28);
	}

	if(loopStart!=0) {
		if(vgmstream->channels==1) {
			vgmstream->loop_start_sample = loopStart/16*18;
			vgmstream->loop_end_sample = loopEnd/16*28;
		} else {
			vgmstream->loop_start_sample = ((loopStart/(interleave*channel_count))*interleave)/16*14*(2/channel_count);
			vgmstream->loop_start_sample += (loopStart%(interleave*channel_count))/16*14*(2/channel_count);
			vgmstream->loop_end_sample = ((loopEnd/(interleave*channel_count))*interleave)/16*28*(2/channel_count);
			vgmstream->loop_end_sample += (loopEnd%(interleave*channel_count))/16*14*(2/channel_count);
		}
	}

	vgmstream->coding_type = coding_PSX;
    vgmstream->layout_type = layout_interleave;
    
	vgmstream->meta_type = meta_PS2_MIB;
    
	if (gotMIH) {
		vgmstream->meta_type = meta_PS2_MIB_MIH;
		close_streamfile(streamFileMIH); streamFileMIH=NULL;
	}

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

            if (!vgmstream->ch[i].streamfile) goto fail;

            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=i*vgmstream->interleave_block_size;
        }
    }

    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (streamFileMIH) close_streamfile(streamFileMIH);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

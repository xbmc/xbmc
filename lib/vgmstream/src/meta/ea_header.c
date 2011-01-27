#include "meta.h"
#include "../layout/layout.h"
#include "../coding/coding.h"
#include "../util.h"

// Platform constants
#define EA_PC	0x00
#define EA_PSX	0x01
#define EA_PS2	0x05
#define EA_GC	0x06
#define EA_XBOX	0x07
#define EA_X360	0x09

// Compression Version
#define EAXA_R1	0x01
#define EAXA_R2	0x02
#define EAXA_R3	0x03

// Compression Type
#define EA_VAG		0x01
#define EA_EAXA		0x0A
#define EA_ADPCM	0x30
#define EA_PCM_BE	0x07
#define EA_PCM_LE	0x08
#define EA_IMA		0x14

typedef struct {
    int32_t num_samples;    
    int32_t sample_rate;    
    uint8_t channels;           
    uint8_t platform;   
	int32_t	interleave;
	uint8_t	compression_type;
	uint8_t	compression_version;
} EA_STRUCT;

uint32_t readPatch(STREAMFILE* streamFile, off_t* offset) {
	
	uint32_t	result=0;
	uint8_t		byteCount;

	byteCount = read_8bit(*offset,streamFile);
	(*offset)++;
	
	for(;byteCount>0;byteCount--) {
		result <<=8;
		result+=(uint8_t)read_8bit(*offset,streamFile);
		(*offset)++;
	}
	return result;
}

void Parse_Header(STREAMFILE* streamFile,EA_STRUCT* ea, off_t offset, int length) {

	uint8_t byteRead;
	off_t	begin_offset=offset;

	// default value ...
	ea->channels=1;
	ea->compression_type=0;
	ea->compression_version=0x01;
	ea->platform=EA_GC;

	if(read_32bitBE(offset, streamFile)==0x47535452) { // GSTR
		ea->compression_version=0x03;
		offset+=8;
		ea->platform=6;
	} else {
		if(read_16bitBE(offset,streamFile)!=0x5054)  // PT
			offset+=4;

		ea->platform=(uint8_t)read_16bitLE(offset+2,streamFile);
		offset+=4;
	}

	do {
		byteRead = read_8bit(offset++,streamFile);

		switch(byteRead) {
			case 0xFF:
			case 0xFE:
			case 0xFC:
			case 0xFD:
				break;
			case 0x80: // compression version
				ea->compression_version = (uint8_t)readPatch(streamFile, &offset);
				break;
			case 0x82: // channels count
				ea->channels = (uint8_t)readPatch(streamFile, &offset);
				break;
			case 0x83: // compression type
				ea->compression_type = (uint8_t)readPatch(streamFile, &offset);
				if(ea->compression_type==0x07) ea->compression_type=0x30;
				break;
			case 0x84: // sample frequency
				ea->sample_rate = readPatch(streamFile,&offset);
				break;
			case 0x85: // samples count
				ea->num_samples = readPatch(streamFile, &offset);
				break;
			case 0x8A:
				offset+=4;
				if(ea->compression_type==0) ea->compression_type=EA_PCM_LE;
				break;
			case 0x86:
			case 0x87:
			case 0x8C:
			case 0x92:
			case 0x9C:
			case 0x9D: // unknown patch
				readPatch(streamFile, &offset);
				break;
			case 0x88: // interleave
				ea->interleave = readPatch(streamFile, &offset);
				break;
			case 0xA0: // compression type
				ea->compression_type = (uint8_t)readPatch(streamFile, &offset);
				break;
		}
	} while(offset-begin_offset<length);

	if(ea->platform==EA_PSX)
		ea->compression_type=EA_VAG;
	if(ea->compression_type==0)
		ea->compression_type=EA_EAXA;
}

VGMSTREAM * init_vgmstream_ea(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
	EA_STRUCT ea;
    char filename[260];

    int loop_flag=0;
    int channel_count;
	int header_length;
	off_t	start_offset;
    int i;

	memset(&ea,0,sizeof(EA_STRUCT));

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("sng",filename_extension(filename)) && 
		strcasecmp("asf",filename_extension(filename)) && 
		strcasecmp("str",filename_extension(filename)) && 
		strcasecmp("xsf",filename_extension(filename)) && 
		strcasecmp("eam",filename_extension(filename))) goto fail;

    /* check Header */
    if (read_32bitBE(0x00,streamFile) != 0x5343486C) // SCHl
        goto fail;

	header_length = read_32bitLE(0x04,streamFile);
	start_offset=8;

	if(header_length>0x100) goto fail;

	Parse_Header(streamFile,&ea,start_offset,header_length-8);

    /* unknown loop value for the moment */
    loop_flag = 0;

    channel_count=ea.channels;

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    vgmstream->channels = channel_count;
	vgmstream->ea_platform=ea.platform;

	vgmstream->ea_compression_type=ea.compression_type;
	vgmstream->ea_compression_version=ea.compression_version;

	// Set defaut sample rate if not define in the header
	if(ea.sample_rate!=0) {
		vgmstream->sample_rate = ea.sample_rate;
	} else {
		if(read_32bitBE(0x08,streamFile)==0x47535452) { // GSTR
			vgmstream->sample_rate=44100;
		} else {
			switch(vgmstream->ea_platform) {
				case EA_XBOX:
					vgmstream->sample_rate=24000;
					break;
				case EA_X360:
					vgmstream->sample_rate=44100;
					break;
				default:
					vgmstream->sample_rate=22050;
			}
		}
	}

	// Set default compression scheme if not define in the header
	switch(vgmstream->ea_platform) {
		case EA_X360:
			vgmstream->ea_compression_version=0x03;
			break;
	}

	vgmstream->num_samples=ea.num_samples;

	switch(vgmstream->ea_compression_type) {
		case EA_EAXA:
			if(vgmstream->ea_compression_version==0x03)
				vgmstream->meta_type=meta_EAXA_R3;
			else {
				// seems there's no EAXA R2 on PC
				if(ea.platform==EA_PC) {
					vgmstream->ea_compression_version=0x03;
					vgmstream->meta_type=meta_EAXA_R3;
				} else
					vgmstream->meta_type=meta_EAXA_R2;
			}

			vgmstream->coding_type=coding_EAXA;
			vgmstream->layout_type=layout_ea_blocked;
			if((vgmstream->ea_platform==EA_GC) || (vgmstream->ea_platform==EA_X360)) 
				vgmstream->ea_big_endian=1;
			
			break;
		case EA_VAG:
		 	vgmstream->meta_type=meta_EAXA_PSX;
			vgmstream->coding_type=coding_PSX;
			vgmstream->layout_type=layout_ea_blocked;
			break;
		case EA_PCM_LE:
		 	vgmstream->meta_type=meta_EA_PCM;
			vgmstream->coding_type=coding_PCM16LE_int;
			vgmstream->layout_type=layout_ea_blocked;
			break;
		case EA_ADPCM:
		 	vgmstream->meta_type=meta_EA_ADPCM;
			vgmstream->coding_type=coding_EA_ADPCM;
			vgmstream->layout_type=layout_ea_blocked;
			break;
		case EA_IMA:
		 	vgmstream->meta_type=meta_EA_IMA;
			vgmstream->coding_type=coding_XBOX;
			vgmstream->layout_type=layout_ea_blocked;
			break;
	}


    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = streamFile->open(streamFile,filename,0x8000);

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }


	// Special function for .EAM files ...
	if(!strcasecmp("eam",filename_extension(filename))) {

		size_t file_length=get_streamfile_size(streamFile);
		size_t block_length;

		vgmstream->next_block_offset=start_offset+header_length;
		vgmstream->num_samples=0;

		// to initialize the block length
		ea_block_update(start_offset+header_length,vgmstream);
		block_length=vgmstream->next_block_offset-start_offset+header_length;

		do {
			ea_block_update(vgmstream->next_block_offset,vgmstream);
			if(vgmstream->coding_type==coding_PSX) 
				vgmstream->num_samples+=(int32_t)vgmstream->current_block_size/16*28;		
			else if (vgmstream->coding_type==coding_EA_ADPCM)
				vgmstream->num_samples+=(int32_t)vgmstream->current_block_size;
			else if (vgmstream->coding_type==coding_PCM16LE_int)
				vgmstream->num_samples+=(int32_t)vgmstream->current_block_size/vgmstream->channels;
			else
				vgmstream->num_samples+=(int32_t)vgmstream->current_block_size*28;
		} while(vgmstream->next_block_offset<(off_t)(file_length-block_length));
	}

	ea_block_update(start_offset+header_length,vgmstream);

	init_get_high_nibble(vgmstream);

	return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

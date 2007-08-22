/*
cnv_FAAD - MP4-AAC decoder plugin for Winamp3
Copyright (C) 2002 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
	
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
		
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
			
The author can be contacted at:
kreel@tiscali.it
*/

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "resource.h"
#include "faadwa3.h"
#include "cnv_FAAD.h"
#include "CRegistry.h"
#include "Defines.h"



// *********************************************************************************************



#define MAX_Channels	2
#define	FAAD_STREAMSIZE	(FAAD_MIN_STREAMSIZE*MAX_Channels)
/*
#define RAW		0
#define ADIF	1
#define ADTS	2
*/
// -----------------------------------------------------------------------------------------------

#define FREE_ARRAY(ptr) \
{ \
	if(ptr) \
		free(ptr); \
	ptr=0; \
}

// *********************************************************************************************

int id3v2_tag(unsigned char *buffer)
{
	if(StringComp((const char *)buffer, "ID3", 3) == 0)
	{
	unsigned long tagsize;

	// high bit is not used
		tagsize =	(buffer[6] << 21) | (buffer[7] << 14) |
					(buffer[8] <<  7) | (buffer[9] <<  0);
		tagsize += 10;
		return tagsize;
	}
	return 0;
}
// *********************************************************************************************

int GetAACTrack(MP4FileHandle infile)
{
// find AAC track
int i, rc;
int numTracks = MP4GetNumberOfTracks(infile, NULL, 0);

	for (i = 0; i < numTracks; i++)
    {
    MP4TrackId trackId = MP4FindTrackId(infile, i, NULL, 0);
    const char* trackType = MP4GetTrackType(infile, trackId);

        if (!strcmp(trackType, MP4_AUDIO_TRACK_TYPE))
        {
        unsigned char *buff = NULL;
        unsigned __int32 buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;

			MP4GetTrackESConfiguration(infile, trackId, (unsigned __int8 **)&buff, &buff_size);

            if (buff)
            {
                rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
                free(buff);

                if (rc < 0)
                    return -1;
                return trackId;
            }
        }
    }

    // can't decode this
    return -1;
}
// *********************************************************************************************

AacPcm::AacPcm()
{
	mp4File=0;
	aacFile=0;
    hDecoder=0;
    buffer=0;
	bytes_read=0;
	bps=16;
	newpos_ms=-1;
	seek_table=0;
	seek_table_length=0;
	FindBitrate=FALSE;
	BlockSeeking=false;
}
// -----------------------------------------------------------------------------------------------

AacPcm::~AacPcm()
{
	if(mp4File)
		MP4Close(mp4File);
	if(aacFile)
		fclose(aacFile);
	if(hDecoder)
		faacDecClose(hDecoder);
	FREE_ARRAY(buffer);
	FREE_ARRAY(seek_table);
}

// *********************************************************************************************

#define STRING_MONO		"%i kbit/s %i khz %i bps Mono"
#define STRING_STEREO	"%i kbit/s %i khz %i bps Stereo"

#define SHOW_INFO() \
{ \
	infos->setInfo(StringPrintf(Channels==1 ? STRING_MONO : STRING_STEREO, \
								(int)file_info.bitrate/1000, (int)Samplerate/1000, (int)bps)); \
	infos->setTitle(Std::filename(infos->getFilename())); \
	infos->setLength(len_ms); \
}

// -----------------------------------------------------------------------------------------------

#define ERROR_getInfos(str) \
{ \
	bytes_into_buffer=-1; \
	if(str) \
		infos->warning(str); \
	return 1; \
}
// -----------------------------------------------------------------------------------------------

int AacPcm::getInfos(MediaInfo *infos)
{
	if(!infos)
		return 1;

	if(hDecoder)
	{
		SHOW_INFO()
	    return 0;
	}

	IsAAC=strcmpi(infos->getFilename()+lstrlen(infos->getFilename())-4,".aac")==0;

	if(!IsAAC) // MP4 file ---------------------------------------------------------------------
	{
	MP4Duration			length;
	unsigned __int32	buffer_size;
    mp4AudioSpecificConfig mp4ASC;

		if(!(mp4File=MP4Read(infos->getFilename(), 0)))
			ERROR_getInfos("Error opening file");

		if((track=GetAACTrack(mp4File))<0)
			ERROR_getInfos(0); //"Unable to find correct AAC sound track");

		if(!(hDecoder=faacDecOpen()))
			ERROR_getInfos("Error initializing decoder library");

		MP4GetTrackESConfiguration(mp4File, track, (unsigned __int8 **)&buffer, &buffer_size);
		if(!buffer)
			ERROR_getInfos("MP4GetTrackESConfiguration");
		AudioSpecificConfig(buffer, buffer_size, &mp4ASC);
        Channels=mp4ASC.channelsConfiguration;

		if(faacDecInit2(hDecoder, buffer, buffer_size, &Samplerate, &Channels) < 0)
			ERROR_getInfos("Error initializing decoder library");
		FREE_ARRAY(buffer);

		length=MP4GetTrackDuration(mp4File, track);
		len_ms=(DWORD)MP4ConvertFromTrackDuration(mp4File, track, length, MP4_MSECS_TIME_SCALE);
		file_info.bitrate=MP4GetTrackBitRate(mp4File, track);
		file_info.version=MP4GetTrackAudioType(mp4File, track)==MP4_MPEG4_AUDIO_TYPE ? 4 : 2;
		numSamples=MP4GetTrackNumberOfSamples(mp4File, track);
		sampleId=1;
	}
	else // AAC file ------------------------------------------------------------------------------
	{   
	DWORD			read,
					tmp;
	BYTE			Channels4Raw=0;

		if(!(aacFile=fopen(infos->getFilename(),"rb")))
			ERROR_getInfos("Error opening file"); 

		// use bufferized stream
		setvbuf(aacFile,NULL,_IOFBF,32767);

		// get size of file
		fseek(aacFile, 0, SEEK_END);
		src_size=ftell(aacFile);
		fseek(aacFile, 0, SEEK_SET);

		if(!(buffer=(BYTE *)malloc(FAAD_STREAMSIZE)))
			ERROR_getInfos("Memory allocation error: buffer")

		tmp=src_size<FAAD_STREAMSIZE ? src_size : FAAD_STREAMSIZE;
		read=fread(buffer, 1, tmp, aacFile);
		if(read==tmp)
		{
			bytes_read=read;
			bytes_into_buffer=read;
		}
		else
			ERROR_getInfos("Read failed!")

		if(tagsize=id3v2_tag(buffer))
		{
			if(tagsize>(long)src_size)
				ERROR_getInfos("Corrupt stream!");
			if(tagsize<bytes_into_buffer)
			{
				bytes_into_buffer-=tagsize;
				memcpy(buffer,buffer+tagsize,bytes_into_buffer);
			}
			else
			{
				bytes_read=tagsize;
				bytes_into_buffer=0;
				if(tagsize>bytes_into_buffer)
					fseek(aacFile, tagsize, SEEK_SET);
			}
			if(src_size<bytes_read+FAAD_STREAMSIZE-bytes_into_buffer)
				tmp=src_size-bytes_read;
			else
				tmp=FAAD_STREAMSIZE-bytes_into_buffer;
			read=fread(buffer+bytes_into_buffer, 1, tmp, aacFile);
			if(read==tmp)
			{
				bytes_read+=read;
				bytes_into_buffer+=read;
			}
			else
				ERROR_getInfos("Read failed!");
		}

		if(get_AAC_format((char *)infos->getFilename(), &file_info, &seek_table, &seek_table_length, 0))
			ERROR_getInfos("get_AAC_format");
		IsSeekable=file_info.headertype==ADTS && seek_table && seek_table_length>0;
		BlockSeeking=!IsSeekable;

		if(!(hDecoder=faacDecOpen()))
			ERROR_getInfos("Can't open library");

		if(file_info.headertype==RAW)
		{
		faacDecConfiguration	config;

			config.defSampleRate=atoi(cfg_samplerate);
			switch(cfg_profile[1])
			{
			case 'a':
				config.defObjectType=MAIN;
				break;
			case 'o':
				config.defObjectType=LOW;
				break;
			case 'S':
				config.defObjectType=SSR;
				break;
			case 'T':
				config.defObjectType=LTP;
				break;
			}
			switch(cfg_bps[0])
			{
			case '1':
				config.outputFormat=FAAD_FMT_16BIT;
				break;
			case '2':
				config.outputFormat=FAAD_FMT_24BIT;
				break;
			case '3':
				config.outputFormat=FAAD_FMT_32BIT;
				break;
			case 'F':
				config.outputFormat=FAAD_FMT_24BIT;
				break;
			}
			faacDecSetConfiguration(hDecoder, &config);

			if(!FindBitrate)
			{
			AacPcm *NewInst;
				if(!(NewInst=new AacPcm()))
					ERROR_getInfos("Memory allocation error: NewInst");

				NewInst->FindBitrate=TRUE;
				if(NewInst->getInfos(infos))
					ERROR_getInfos(0);
				Channels4Raw=NewInst->frameInfo.channels;
				file_info.bitrate=NewInst->file_info.bitrate*Channels4Raw;
				delete NewInst;
			}
			else
			{
			DWORD	Samples,
					BytesConsumed;

				if((bytes_consumed=faacDecInit(hDecoder,buffer,bytes_into_buffer,&Samplerate,&Channels))<0)
					ERROR_getInfos("Can't init library");
				bytes_into_buffer-=bytes_consumed;
				if(!processData(infos,0,0))
					ERROR_getInfos(0);
				Samples=frameInfo.samples/sizeof(short);
				BytesConsumed=frameInfo.bytesconsumed;
				processData(infos,0,0);
				if(BytesConsumed<frameInfo.bytesconsumed)
					BytesConsumed=frameInfo.bytesconsumed;
				file_info.bitrate=(BytesConsumed*8*Samplerate)/Samples;
				if(!file_info.bitrate)
					file_info.bitrate=1000; // try to continue decoding
				return 0;
			}
		}

		if((bytes_consumed=faacDecInit(hDecoder, buffer, bytes_into_buffer, &Samplerate, &Channels))<0)
			ERROR_getInfos("faacDecInit failed!")
		bytes_into_buffer-=bytes_consumed;

		if(Channels4Raw)
			Channels=Channels4Raw;

		len_ms=(DWORD)((1000*((float)src_size*8))/file_info.bitrate);
	}

	SHOW_INFO();
    return 0;
}
// *********************************************************************************************

#define ERROR_processData(str) \
{ \
	bytes_into_buffer=-1; \
	if(str) \
		infos->warning(str); \
	if(chunk_list) \
		chunk_list->setChunk("PCM", bufout, 0, ci); \
	return 0; \
}
// -----------------------------------------------------------------------------------------------

int AacPcm::processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch)
{
DWORD			BytesDecoded=0;
char			*bufout=0;
ChunkInfosI		*ci=0;
svc_fileReader	*reader=0;

	if(!FindBitrate && !chunk_list)
		ERROR_processData("chunk_list==NULL"); // is this case possible?

	if(!(reader=infos->getReader()))
		ERROR_processData("File doesn\'t exists");

	if(chunk_list)
	{
		if(!(ci=new ChunkInfosI()))
			ERROR_processData("Memory allocation error: ci");

		ci->addInfo("srate", Samplerate);
		ci->addInfo("bps", bps);
		ci->addInfo("nch", Channels);
	}

	if(!IsAAC) // MP4 file --------------------------------------------------------------------------
	{   
	unsigned __int32 buffer_size=0;
    int rc;

		if(newpos_ms>-1)
		{
		MP4Duration duration=MP4ConvertToTrackDuration(mp4File,track,newpos_ms,MP4_MSECS_TIME_SCALE);
            sampleId=MP4GetSampleIdFromTime(mp4File,track,duration,0);
			bytes_read=(DWORD)(((float)newpos_ms*file_info.bitrate)/(8*1000));
			reader->seek(bytes_read);  // updates slider
			newpos_ms=-1;
		}
		do
		{
			buffer=NULL;
			if(sampleId>=numSamples)
				ERROR_processData(0);

			rc=MP4ReadSample(mp4File, track, sampleId++, (unsigned __int8 **)&buffer, &buffer_size, NULL, NULL, NULL, NULL);
			if(rc==0 || buffer==NULL)
			{
				FREE_ARRAY(buffer);
				ERROR_processData("MP4ReadSample")
			}

			bufout=(char *)faacDecDecode(hDecoder,&frameInfo,buffer,buffer_size);
			BytesDecoded=frameInfo.samples*sizeof(short);
			FREE_ARRAY(buffer);
			// to update the slider
			bytes_read+=buffer_size;
			reader->seek(bytes_read);
		}while(!BytesDecoded && !frameInfo.error);
	}
	else // AAC file --------------------------------------------------------------------------
	{   
	DWORD	read,
			tmp;

		if(BlockSeeking)
		{
			infos->setLength(-1);
			BlockSeeking=false;
		}
		if(newpos_ms>-1)
		{
			if(IsSeekable)
			{
			DWORD normalized=len_ms/(seek_table_length-1);
				if(normalized<1000)
					normalized=1000;
				bytes_read=seek_table[newpos_ms/normalized];
				fseek(aacFile, bytes_read, SEEK_SET);
				reader->seek(bytes_read); // updates slider
				bytes_into_buffer=0;
				bytes_consumed=FAAD_STREAMSIZE;
			}
			newpos_ms=-1;
		}
		do
		{
			if(bytes_consumed>0 && bytes_into_buffer>=0)
			{
				if(bytes_into_buffer)
					memcpy(buffer,buffer+bytes_consumed,bytes_into_buffer);

				if(bytes_read<src_size)
				{
					if(bytes_read+bytes_consumed<src_size)
						tmp=bytes_consumed;
					else
						tmp=src_size-bytes_read;
					read=fread(buffer+bytes_into_buffer, 1, tmp, aacFile);
					if(read==tmp)
					{
						bytes_read+=read;
						bytes_into_buffer+=read;
					}
					else
						infos->status("Read failed!"); // continue until bytes_into_buffer<1
				}
				else
					if(bytes_into_buffer)
						memset(buffer+bytes_into_buffer, 0, bytes_consumed);

				bytes_consumed=0;
			}

			if(bytes_into_buffer<1)
				if(bytes_read<src_size)
					ERROR_processData("Buffer empty!")
				else
					ERROR_processData(0);

			bufout=(char *)faacDecDecode(hDecoder,&frameInfo,buffer,bytes_into_buffer);
			BytesDecoded=frameInfo.samples*sizeof(short);
			bytes_consumed+=frameInfo.bytesconsumed;
			bytes_into_buffer-=bytes_consumed;
			// to update the slider
			reader->seek(bytes_read-bytes_into_buffer+bytes_consumed); // updates slider
		}while(!BytesDecoded && !frameInfo.error);
	} // END AAC file --------------------------------------------------------------------------

	if(frameInfo.error)
		ERROR_processData((char *)faacDecGetErrorMessage(frameInfo.error));

	if(chunk_list)
		chunk_list->setChunk("PCM", bufout, BytesDecoded, ci);
    return 1;
}

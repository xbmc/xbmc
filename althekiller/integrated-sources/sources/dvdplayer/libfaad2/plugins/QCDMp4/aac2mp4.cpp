/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: aac2mp4.cpp,v 1.3 2003/12/06 04:24:17 rjamorim Exp $
**/

#include <mpeg4ip.h>
#include <mp4.h>
#include <mp4av.h>

#include "aac2mp4.h"

int covert_aac_to_mp4(char *inputFileName, char *mp4FileName)
{
    int Mp4TimeScale = 90000;
    int allMpeg4Streams = 0;
    MP4FileHandle mp4File;
    FILE* inFile;
    const char *type;
    MP4TrackId createdTrackId = MP4_INVALID_TRACK_ID;

    mp4File = MP4Create(mp4FileName, 0, 0, 0);
    if (mp4File)
    {
        MP4SetTimeScale(mp4File, Mp4TimeScale);
    } else {
        return 1;
    }

    inFile = fopen(inputFileName, "rb");

	if (inFile == NULL)
    {
        MP4Close(mp4File);
        return 2;
    }

    createdTrackId = AacCreator(mp4File, inFile);

    if (createdTrackId == MP4_INVALID_TRACK_ID)
    {
        fclose(inFile);
        MP4Close(mp4File);
        return 3;
    }

    type = MP4GetTrackType(mp4File, createdTrackId);

    if (!strcmp(type, MP4_AUDIO_TRACK_TYPE))
    {
        allMpeg4Streams &=
            (MP4GetTrackAudioType(mp4File, createdTrackId)
            == MP4_MPEG4_AUDIO_TYPE);
    }

    if (inFile)
    {
        fclose(inFile);
    }

    MP4Close(mp4File);
    MP4MakeIsmaCompliant(mp4FileName, 0, allMpeg4Streams);

    return 0;
}

#define ADTS_HEADER_MAX_SIZE 10 /* bytes */

static u_int8_t firstHeader[ADTS_HEADER_MAX_SIZE];

/* 
 * hdr must point to at least ADTS_HEADER_MAX_SIZE bytes of memory 
 */
static bool LoadNextAdtsHeader(FILE* inFile, u_int8_t* hdr)
{
	u_int state = 0;
	u_int dropped = 0;
	u_int hdrByteSize = ADTS_HEADER_MAX_SIZE;

	while (1) {
		/* read a byte */
		u_int8_t b;

		if (fread(&b, 1, 1, inFile) == 0) {
			return false;
		}

		/* header is complete, return it */
		if (state == hdrByteSize - 1) {
			hdr[state] = b;
			if (dropped > 0) {
				fprintf(stderr, "Warning: dropped %u input bytes\n", dropped);
			}
			return true;
		}

		/* collect requisite number of bytes, no constraints on data */
		if (state >= 2) {
			hdr[state++] = b;
		} else {
			/* have first byte, check if we have 1111X00X */
			if (state == 1) {
				if ((b & 0xF6) == 0xF0) {
					hdr[state] = b;
					state = 2;
					/* compute desired header size */
					hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdr);
				} else {
					state = 0;
				}
			}
			/* initial state, looking for 11111111 */
			if (state == 0) {
				if (b == 0xFF) {
					hdr[state] = b;
					state = 1;
				} else {
					 /* else drop it */ 
					dropped++;
				}
			}
		}
	}
}

/*
 * Load the next frame from the file
 * into the supplied buffer, which better be large enough!
 *
 * Note: Frames are padded to byte boundaries
 */
static bool LoadNextAacFrame(FILE* inFile, u_int8_t* pBuf, u_int32_t* pBufSize, bool stripAdts)
{
	u_int16_t frameSize;
	u_int16_t hdrBitSize, hdrByteSize;
	u_int8_t hdrBuf[ADTS_HEADER_MAX_SIZE];

	/* get the next AAC frame header, more or less */
	if (!LoadNextAdtsHeader(inFile, hdrBuf)) {
		return false;
	}
	
	/* get frame size from header */
	frameSize = MP4AV_AdtsGetFrameSize(hdrBuf);

	/* get header size in bits and bytes from header */
	hdrBitSize = MP4AV_AdtsGetHeaderBitSize(hdrBuf);
	hdrByteSize = MP4AV_AdtsGetHeaderByteSize(hdrBuf);
	
	/* adjust the frame size to what remains to be read */
	frameSize -= hdrByteSize;

	if (stripAdts) {
		if ((hdrBitSize % 8) == 0) {
			/* header is byte aligned, i.e. MPEG-2 ADTS */
			/* read the frame data into the buffer */
			if (fread(pBuf, 1, frameSize, inFile) != frameSize) {
				return false;
			}
			(*pBufSize) = frameSize;
		} else {
			/* header is not byte aligned, i.e. MPEG-4 ADTS */
			int i;
			u_int8_t newByte;
			int upShift = hdrBitSize % 8;
			int downShift = 8 - upShift;

			pBuf[0] = hdrBuf[hdrBitSize / 8] << upShift;

			for (i = 0; i < frameSize; i++) {
				if (fread(&newByte, 1, 1, inFile) != 1) {
					return false;
				}
				pBuf[i] |= (newByte >> downShift);
				pBuf[i+1] = (newByte << upShift);
			}
			(*pBufSize) = frameSize + 1;
		}
	} else { /* don't strip ADTS headers */
		memcpy(pBuf, hdrBuf, hdrByteSize);
		if (fread(&pBuf[hdrByteSize], 1, frameSize, inFile) != frameSize) {
			return false;
		}
	}

	return true;
}

static bool GetFirstHeader(FILE* inFile)
{
	/* read file until we find an audio frame */
	fpos_t curPos;

	/* already read first header */
	if (firstHeader[0] == 0xff) {
		return true;
	}

	/* remember where we are */
	fgetpos(inFile, &curPos);
	
	/* go back to start of file */
	rewind(inFile);

	if (!LoadNextAdtsHeader(inFile, firstHeader)) {
		return false;
	}

	/* reposition the file to where we were */
	fsetpos(inFile, &curPos);

	return true;
}

MP4TrackId AacCreator(MP4FileHandle mp4File, FILE* inFile)
{
	// collect all the necessary meta information
	u_int32_t samplesPerSecond;
	u_int8_t mpegVersion;
	u_int8_t profile;
	u_int8_t channelConfig;

	if (!GetFirstHeader(inFile)) {
		return MP4_INVALID_TRACK_ID;
	}

	samplesPerSecond = MP4AV_AdtsGetSamplingRate(firstHeader);
	mpegVersion = MP4AV_AdtsGetVersion(firstHeader);
	profile = MP4AV_AdtsGetProfile(firstHeader);
	channelConfig = MP4AV_AdtsGetChannels(firstHeader);

	u_int8_t audioType = MP4_INVALID_AUDIO_TYPE;
	switch (mpegVersion) {
	case 0:
		audioType = MP4_MPEG4_AUDIO_TYPE;
		break;
	case 1:
		switch (profile) {
		case 0:
			audioType = MP4_MPEG2_AAC_MAIN_AUDIO_TYPE;
			break;
		case 1:
			audioType = MP4_MPEG2_AAC_LC_AUDIO_TYPE;
			break;
		case 2:
			audioType = MP4_MPEG2_AAC_SSR_AUDIO_TYPE;
			break;
		case 3:
			return MP4_INVALID_TRACK_ID;
		}
		break;
	}

	// add the new audio track
	MP4TrackId trackId = 
		MP4AddAudioTrack(mp4File, 
			samplesPerSecond, 1024, audioType);

	if (trackId == MP4_INVALID_TRACK_ID) {
		return MP4_INVALID_TRACK_ID;
	}

	if (MP4GetNumberOfTracks(mp4File, MP4_AUDIO_TRACK_TYPE) == 1) {
		MP4SetAudioProfileLevel(mp4File, 0x0F);
	}

	u_int8_t* pConfig = NULL;
	u_int32_t configLength = 0;

	MP4AV_AacGetConfiguration(
		&pConfig,
		&configLength,
		profile,
		samplesPerSecond,
		channelConfig);

	if (!MP4SetTrackESConfiguration(mp4File, trackId, 
	  pConfig, configLength)) {
		MP4DeleteTrack(mp4File, trackId);
		return MP4_INVALID_TRACK_ID;
	}

	// parse the ADTS frames, and write the MP4 samples
	u_int8_t sampleBuffer[8 * 1024];
	u_int32_t sampleSize = sizeof(sampleBuffer);
	MP4SampleId sampleId = 1;

	while (LoadNextAacFrame(inFile, sampleBuffer, &sampleSize, true)) {
		if (!MP4WriteSample(mp4File, trackId, sampleBuffer, sampleSize)) {
			MP4DeleteTrack(mp4File, trackId);
			return MP4_INVALID_TRACK_ID;
		}
		sampleId++;
		sampleSize = sizeof(sampleBuffer);
	}

	return trackId;
}

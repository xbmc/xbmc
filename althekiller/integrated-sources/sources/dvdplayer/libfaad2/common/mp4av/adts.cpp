/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

/* 
 * Notes:
 *  - file formatted with tabstops == 4 spaces 
 */

#include <mp4av_common.h>

/*
 * ADTS Header: 
 *  MPEG-2 version 56 bits (byte aligned) 
 *  MPEG-4 version 56 bits (byte aligned) - note - changed for 0.99 version
 *
 * syncword						12 bits
 * id							1 bit
 * layer						2 bits
 * protection_absent			1 bit
 * profile						2 bits
 * sampling_frequency_index		4 bits
 * private						1 bit
 * channel_configuraton			3 bits
 * original						1 bit
 * home							1 bit
 * copyright_id					1 bit
 * copyright_id_start			1 bit
 * aac_frame_length				13 bits
 * adts_buffer_fullness			11 bits
 * num_raw_data_blocks			2 bits
 *
 * if (protection_absent == 0)
 *	crc_check					16 bits
 */

u_int32_t AdtsSamplingRates[NUM_ADTS_SAMPLING_RATES] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
	16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

/*
 * compute ADTS frame size
 */
extern "C" u_int16_t MP4AV_AdtsGetFrameSize(u_int8_t* pHdr)
{
	/* extract the necessary fields from the header */
	uint16_t frameLength;

	frameLength = (((u_int16_t)(pHdr[3] & 0x3)) << 11) 
	  | (((u_int16_t)pHdr[4]) << 3) | (pHdr[5] >> 5); 

	return frameLength;
}

/*
 * Compute length of ADTS header in bits
 */
extern "C" u_int16_t MP4AV_AdtsGetHeaderBitSize(u_int8_t* pHdr)
{
	u_int8_t hasCrc = !(pHdr[1] & 0x01);
	u_int16_t hdrSize;

	hdrSize = 56;

	if (hasCrc) {
		hdrSize += 16;
	}
	return hdrSize;
}

extern "C" u_int16_t MP4AV_AdtsGetHeaderByteSize(u_int8_t* pHdr)
{
	return (MP4AV_AdtsGetHeaderBitSize(pHdr) + 7) / 8;
}

extern "C" u_int8_t MP4AV_AdtsGetVersion(u_int8_t* pHdr)
{
	return (pHdr[1] & 0x08) >> 3;
}

extern "C" u_int8_t MP4AV_AdtsGetProfile(u_int8_t* pHdr)
{
	return (pHdr[2] & 0xc0) >> 6;
}

extern "C" u_int8_t MP4AV_AdtsGetSamplingRateIndex(u_int8_t* pHdr)
{
	return (pHdr[2] & 0x3c) >> 2;
}

extern "C" u_int8_t MP4AV_AdtsFindSamplingRateIndex(u_int32_t samplingRate)
{
	for (u_int8_t i = 0; i < NUM_ADTS_SAMPLING_RATES; i++) {
		if (samplingRate == AdtsSamplingRates[i]) {
			return i;
		}
	}
	return NUM_ADTS_SAMPLING_RATES - 1;
}

extern "C" u_int32_t MP4AV_AdtsGetSamplingRate(u_int8_t* pHdr)
{
	return AdtsSamplingRates[MP4AV_AdtsGetSamplingRateIndex(pHdr)];
}

extern "C" u_int8_t MP4AV_AdtsGetChannels(u_int8_t* pHdr)
{
	return ((pHdr[2] & 0x1) << 2) | ((pHdr[3] & 0xc0) >> 6);
}

extern "C" bool MP4AV_AdtsMakeFrameFromMp4Sample(
	MP4FileHandle mp4File,
	MP4TrackId trackId,
	MP4SampleId sampleId,
	int force_profile,
	u_int8_t** ppAdtsData,
	u_int32_t* pAdtsDataLength)
{
	static MP4FileHandle lastMp4File = MP4_INVALID_FILE_HANDLE;
	static MP4TrackId lastMp4TrackId = MP4_INVALID_TRACK_ID;
	static bool isMpeg2;
	static u_int8_t profile;
	static u_int32_t samplingFrequency;
	static u_int8_t channels;

	if (mp4File != lastMp4File || trackId != lastMp4TrackId) {

		// changed cached file and track info

		lastMp4File = mp4File;
		lastMp4TrackId = trackId;

		u_int8_t audioType = MP4GetTrackEsdsObjectTypeId(mp4File, 
								 trackId);

		if (MP4_IS_MPEG2_AAC_AUDIO_TYPE(audioType)) {
			isMpeg2 = true;
			profile = audioType - MP4_MPEG2_AAC_MAIN_AUDIO_TYPE;
			if (force_profile == 4) {
			  isMpeg2 = false;
			  // profile remains the same
			}
		} else if (audioType == MP4_MPEG4_AUDIO_TYPE) {
			isMpeg2 = false;
			profile = MP4GetTrackAudioMpeg4Type(mp4File, trackId) - 1;
			if (force_profile == 2) {
			  if (profile > MP4_MPEG4_AAC_SSR_AUDIO_TYPE) {
			    // they can't use these profiles for mpeg2.
			    lastMp4File = MP4_INVALID_FILE_HANDLE;
			    lastMp4TrackId =MP4_INVALID_TRACK_ID;
			    return false;
			  }
			  isMpeg2 = true;
			}
		} else {
			lastMp4File = MP4_INVALID_FILE_HANDLE;
			lastMp4TrackId = MP4_INVALID_TRACK_ID;
			return false;
		}

		u_int8_t* pConfig = NULL;
		u_int32_t configLength;

		MP4GetTrackESConfiguration(
			mp4File, 
			trackId,
			&pConfig,
			&configLength);

		if (pConfig == NULL || configLength < 2) {
			lastMp4File = MP4_INVALID_FILE_HANDLE;
			lastMp4TrackId = MP4_INVALID_TRACK_ID;
			return false;
		}

		samplingFrequency = MP4AV_AacConfigGetSamplingRate(pConfig);

		channels = MP4AV_AacConfigGetChannels(pConfig);

	}

	bool rc;
	u_int8_t* pSample = NULL;
	u_int32_t sampleSize = 0;

	rc = MP4ReadSample(
		mp4File,
		trackId,
		sampleId,
		&pSample,
		&sampleSize);

	if (!rc) {
		return false;
	}

	rc = MP4AV_AdtsMakeFrame(
		pSample,
		sampleSize,
		isMpeg2,
		profile,
		samplingFrequency,
		channels,
		ppAdtsData,
		pAdtsDataLength);

	free(pSample);

	return rc;
}

extern "C" bool MP4AV_AdtsMakeFrame(
	u_int8_t* pData,
	u_int16_t dataLength,
	bool isMpeg2,
	u_int8_t profile,
	u_int32_t samplingFrequency,
	u_int8_t channels,
	u_int8_t** ppAdtsData,
	u_int32_t* pAdtsDataLength)
{
	*pAdtsDataLength = 7 + dataLength; // 56 bits only

	CMemoryBitstream adts;

	try {
		adts.AllocBytes(*pAdtsDataLength);
		*ppAdtsData = adts.GetBuffer();

		// build adts header
		adts.PutBits(0xFFF, 12);		// syncword
		adts.PutBits(isMpeg2, 1);		// id
		adts.PutBits(0, 2);				// layer
		adts.PutBits(1, 1);				// protection_absent
		adts.PutBits(profile, 2);		// profile
		adts.PutBits(
			MP4AV_AdtsFindSamplingRateIndex(samplingFrequency),
			4);							// sampling_frequency_index
		adts.PutBits(0, 1);				// private
		adts.PutBits(channels, 3);		// channel_configuration
		adts.PutBits(0, 1);				// original
		adts.PutBits(0, 1);				// home

		adts.PutBits(0, 1);				// copyright_id
		adts.PutBits(0, 1);				// copyright_id_start
		adts.PutBits(*pAdtsDataLength, 13);	// aac_frame_length
		adts.PutBits(0x7FF, 11);		// adts_buffer_fullness
		adts.PutBits(0, 2);				// num_raw_data_blocks

		// copy audio frame data
		adts.PutBytes(pData, dataLength);
	}
	catch (int e) {
		return false;
	}

	return true;
}


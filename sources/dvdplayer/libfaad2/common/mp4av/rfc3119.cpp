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

// LATER get these on the stack so library is thread-safe!
// file globals
static bool doInterleave;
static u_int32_t samplesPerPacket;
static u_int32_t samplesPerGroup;
static MP4AV_Mp3Header* pFrameHeaders = NULL;
static u_int16_t* pAduOffsets = NULL;

static bool GetFrameInfo(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId,
	MP4AV_Mp3Header** ppFrameHeaders,
	u_int16_t** ppAduOffsets)
{
	// allocate memory to hold the frame info that we need
	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	*ppFrameHeaders = 
		(MP4AV_Mp3Header*)calloc((numSamples + 2), sizeof(u_int32_t));
	if (*ppFrameHeaders == NULL) {
		return false;
	}

	*ppAduOffsets = 
		(u_int16_t*)calloc((numSamples + 2), sizeof(u_int16_t));
	if (*ppAduOffsets == NULL) {
		free(*ppFrameHeaders);
		return false;
	}

	// for each sample
	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) { 
		u_int8_t* pSample = NULL;
		u_int32_t sampleSize = 0;

		// read it
		MP4ReadSample(
			mp4File,
			mediaTrackId,
			sampleId,
			&pSample,
			&sampleSize);

		// extract the MP3 frame header
		MP4AV_Mp3Header mp3hdr = 
			MP4AV_Mp3HeaderFromBytes(pSample);

		// store what we need
		(*ppFrameHeaders)[sampleId] = mp3hdr;

		(*ppAduOffsets)[sampleId] = 
			MP4AV_Mp3GetAduOffset(pSample, sampleSize);

		free(pSample);
	}

	return true;
}

static u_int16_t GetFrameHeaderSize(MP4SampleId sampleId)
{
	return 4 + MP4AV_Mp3GetCrcSize(pFrameHeaders[sampleId]) 
		+ MP4AV_Mp3GetSideInfoSize(pFrameHeaders[sampleId]);
}

static u_int16_t GetFrameDataSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId)
{
	return MP4GetSampleSize(mp4File, mediaTrackId, sampleId)
		- GetFrameHeaderSize(sampleId);
}

u_int32_t MP4AV_Rfc3119GetAduSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId)
{
	u_int32_t sampleSize = MP4GetSampleSize(mp4File, mediaTrackId, sampleId);

	return pAduOffsets[sampleId] + sampleSize - pAduOffsets[sampleId + 1];
}

static u_int16_t GetMaxAduSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId)
{
	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	u_int16_t maxAduSize = 0;

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) { 
		u_int16_t aduSize =
			MP4AV_Rfc3119GetAduSize(mp4File, mediaTrackId, sampleId);

		if (aduSize > maxAduSize) {
			maxAduSize = aduSize;
		}
	}

	return maxAduSize;
}

static u_int16_t GetAduDataSize(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId)
{
	return MP4AV_Rfc3119GetAduSize(mp4File, mediaTrackId, sampleId) 
		- GetFrameHeaderSize(sampleId);
}

static void AddFrameHeader(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId)
{
	// when interleaving we replace the 11 bit mp3 frame sync
	if (doInterleave) {
		// compute interleave index and interleave cycle from sampleId
		u_int8_t interleaveIndex =
			(sampleId - 1) % samplesPerGroup;
		u_int8_t interleaveCycle =
			((sampleId - 1) / samplesPerGroup) & 0x7;

		u_int8_t interleaveHeader[4];
		interleaveHeader[0] = 
			interleaveIndex;
		interleaveHeader[1] = 
			(interleaveCycle << 5) | ((pFrameHeaders[sampleId] >> 16) & 0x1F);
		interleaveHeader[2] = 
			(pFrameHeaders[sampleId] >> 8) & 0xFF;
		interleaveHeader[3] = 
			pFrameHeaders[sampleId] & 0xFF;

		MP4AddRtpImmediateData(mp4File, hintTrackId,
			interleaveHeader, 4);

		// add crc and side info from current mp3 frame
		MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId, 4, GetFrameHeaderSize(sampleId) - 4);
	} else {
		// add mp3 header, crc, and side info from current mp3 frame
		MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId, 0, GetFrameHeaderSize(sampleId));
	}
}

static void CollectAduDataBlocks(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId,
	u_int8_t* pNumBlocks,
	u_int32_t** ppOffsets,
	u_int32_t** ppSizes)
{
	// go back from sampleId until 
	// accumulated data bytes can fill sample's ADU
	MP4SampleId sid = sampleId;
	u_int8_t numBlocks = 1;
	u_int32_t prevDataBytes = 0;
	const u_int8_t maxBlocks = 8;

	*ppOffsets = new u_int32_t[maxBlocks];
	*ppSizes = new u_int32_t[maxBlocks];

	(*ppOffsets)[0] = GetFrameHeaderSize(sampleId);
	(*ppSizes)[0] = GetFrameDataSize(mp4File, mediaTrackId, sid);

	while (true) {
		if (prevDataBytes >= pAduOffsets[sampleId]) {
			u_int32_t adjust =
				prevDataBytes - pAduOffsets[sampleId];

			(*ppOffsets)[numBlocks-1] += adjust; 
			(*ppSizes)[numBlocks-1] -= adjust;

			break;
		}
		
		sid--;
		numBlocks++;

		if (sid == 0 || numBlocks > maxBlocks) {
			throw EIO;	// media bitstream error
		}

		(*ppOffsets)[numBlocks-1] = GetFrameHeaderSize(sid);
		(*ppSizes)[numBlocks-1] = GetFrameDataSize(mp4File, mediaTrackId, sid);
		prevDataBytes += (*ppSizes)[numBlocks-1]; 
	}

	*pNumBlocks = numBlocks;
}

bool MP4AV_Rfc3119Concatenator(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration,
	u_int16_t maxPayloadSize)
{
	// handle degenerate case
	if (samplesThisHint == 0) {
		return true;
	}

	// construct the new hint
	MP4AddRtpHint(mp4File, hintTrackId);
	MP4AddRtpPacket(mp4File, hintTrackId, false);

	// rfc 3119 per adu payload header
	u_int8_t payloadHeader[2];

	// for each given sample
	for (u_int8_t i = 0; i < samplesThisHint; i++) {
		MP4SampleId sampleId = pSampleIds[i];

		u_int16_t aduSize = 
			MP4AV_Rfc3119GetAduSize(mp4File, mediaTrackId, sampleId);

		// add the per ADU payload header
		payloadHeader[0] = 0x40 | ((aduSize >> 8) & 0x3F);
		payloadHeader[1] = aduSize & 0xFF;

		MP4AddRtpImmediateData(mp4File, hintTrackId,
			(u_int8_t*)&payloadHeader, sizeof(payloadHeader));
		// add the mp3 frame header and side info
		AddFrameHeader(mp4File, mediaTrackId, hintTrackId, sampleId);

		// collect the info on the adu main data fragments
		u_int8_t numDataBlocks;
		u_int32_t* pDataOffsets;
		u_int32_t* pDataSizes;

		CollectAduDataBlocks(mp4File, mediaTrackId, hintTrackId, sampleId,
			&numDataBlocks, &pDataOffsets, &pDataSizes);

		// collect the needed blocks of data
		u_int16_t dataSize = 0;
		u_int16_t aduDataSize = 
			GetAduDataSize(mp4File, mediaTrackId, sampleId);

		for (int8_t i = numDataBlocks - 1;
		  i >= 0 && dataSize < aduDataSize; i--) {
			u_int32_t blockSize = pDataSizes[i];

			if ((u_int32_t)(aduDataSize - dataSize) < blockSize) {
				blockSize = (u_int32_t)(aduDataSize - dataSize);
			}

			MP4AddRtpSampleData(mp4File, hintTrackId,
				sampleId - i, pDataOffsets[i], blockSize);

			dataSize += blockSize;
		}

		delete [] pDataOffsets;
		delete [] pDataSizes;
	}

	// write the hint
	MP4WriteRtpHint(mp4File, hintTrackId, hintDuration);

	return true;
}

bool MP4AV_Rfc3119Fragmenter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t aduSize, 
	MP4Duration sampleDuration,
	u_int16_t maxPayloadSize)
{
	MP4AddRtpHint(mp4File, hintTrackId);
	MP4AddRtpPacket(mp4File, hintTrackId, false);

	// rfc 3119 payload header
	u_int8_t payloadHeader[2];

	u_int16_t payloadSize = 
		sizeof(payloadHeader) + GetFrameHeaderSize(sampleId);

	// guard against ridiculous payload sizes
	if (payloadSize > maxPayloadSize) {
		return false;
	}

	// add the per ADU fragment payload header
	payloadHeader[0] = 0x40 | ((aduSize >> 8) & 0x3F);
	payloadHeader[1] = aduSize & 0xFF;

	MP4AddRtpImmediateData(mp4File, hintTrackId,
		(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

	payloadHeader[0] |= 0x80;	// mark future packets as continuations

	// add the mp3 frame header and side info
	AddFrameHeader(mp4File, mediaTrackId, hintTrackId, sampleId);

	// collect the info on the adu main data fragments
	u_int8_t numDataBlocks;
	u_int32_t* pDataOffsets;
	u_int32_t* pDataSizes;

	CollectAduDataBlocks(mp4File, mediaTrackId, hintTrackId, sampleId,
		&numDataBlocks, &pDataOffsets, &pDataSizes);

	u_int16_t dataSize = 0;
	u_int16_t aduDataSize = 
		GetAduDataSize(mp4File, mediaTrackId, sampleId);

	// for each data block
	for (int8_t i = numDataBlocks - 1; i >= 0 && dataSize < aduDataSize; i--) {
		u_int32_t blockSize = pDataSizes[i];
		u_int32_t blockOffset = pDataOffsets[i];

		// we may not need all of a block
		if ((u_int32_t)(aduDataSize - dataSize) < blockSize) {
			blockSize = (u_int32_t)(aduDataSize - dataSize);
		}

		dataSize += blockSize;

		// while data left in this block
		while (blockSize > 0) {
			u_int16_t payloadRemaining = maxPayloadSize - payloadSize;

			if (blockSize < payloadRemaining) {
				// the entire block fits in this packet
				MP4AddRtpSampleData(mp4File, hintTrackId,
					sampleId - i, blockOffset, blockSize);

				payloadSize += blockSize;
				blockSize = 0;

			} else {
				// the block fills this packet
				MP4AddRtpSampleData(mp4File, hintTrackId,
					sampleId - i, blockOffset, payloadRemaining);

				blockOffset += payloadRemaining;
				blockSize -= payloadRemaining;

				// start a new RTP packet
				MP4AddRtpPacket(mp4File, hintTrackId, false);

				// add the fragment payload header
				MP4AddRtpImmediateData(mp4File, hintTrackId,
					(u_int8_t*)&payloadHeader, sizeof(payloadHeader));

				payloadSize = sizeof(payloadHeader);
			}
		}
	}

	// write the hint
	MP4WriteRtpHint(mp4File, hintTrackId, sampleDuration);

	// cleanup
	delete [] pDataOffsets;
	delete [] pDataSizes;

	return true;
}

extern "C" bool MP4AV_Rfc3119Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave,
	u_int16_t maxPayloadSize)
{
	int rc;

	u_int32_t numSamples =
		MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

	if (numSamples == 0) {
		return false;
	}

	u_int8_t audioType =
		MP4GetTrackEsdsObjectTypeId(mp4File, mediaTrackId);

	if (!MP4_IS_MP3_AUDIO_TYPE(audioType)) {
		return false;
	}

	MP4Duration sampleDuration = 
		MP4AV_GetAudioSampleDuration(mp4File, mediaTrackId);

	if (sampleDuration == MP4_INVALID_DURATION) {
		return false;
	}

	// choose 500 ms max latency
	MP4Duration maxLatency = 
		MP4GetTrackTimeScale(mp4File, mediaTrackId) / 2;
	if (maxLatency == 0) {
		return false;
	}

	MP4TrackId hintTrackId =
		MP4AddHintTrack(mp4File, mediaTrackId);

	if (hintTrackId == MP4_INVALID_TRACK_ID) {
		return false;
	}

	doInterleave = interleave;

	u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;

	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"mpa-robust", &payloadNumber, 0);

	// load mp3 frame information into memory
	rc = GetFrameInfo(
		mp4File, 
		mediaTrackId, 
		&pFrameHeaders, 
		&pAduOffsets);

	if (!rc) {
		MP4DeleteTrack(mp4File, hintTrackId);
		return false;
	}
 
	if (doInterleave) {
		u_int32_t maxAduSize =
			GetMaxAduSize(mp4File, mediaTrackId);

		// compute how many maximum size samples would fit in a packet
		samplesPerPacket = 
			maxPayloadSize / (maxAduSize + 2);

		// can't interleave if this number is 0 or 1
		if (samplesPerPacket < 2) {
			doInterleave = false;
		}
	}

	if (doInterleave) {
		// initial estimate of samplesPerGroup
		samplesPerGroup = maxLatency / sampleDuration;
		// use that to compute an integral stride
		u_int8_t stride = samplesPerGroup / samplesPerPacket;
		// and then recompute samples per group to deal with rounding
		samplesPerGroup = stride * samplesPerPacket;

		rc = MP4AV_AudioInterleaveHinter(
			mp4File, 
			mediaTrackId, 
			hintTrackId,
			sampleDuration, 
			samplesPerGroup / samplesPerPacket,		// stride
			samplesPerPacket,						// bundle
			maxPayloadSize,
			MP4AV_Rfc3119Concatenator);

	} else {
		rc = MP4AV_AudioConsecutiveHinter(
			mp4File, 
			mediaTrackId, 
			hintTrackId,
			sampleDuration, 
			0,										// perPacketHeaderSize
			2,										// perSampleHeaderSize
			maxLatency / sampleDuration,			// maxSamplesPerPacket
			maxPayloadSize,
			MP4AV_Rfc3119GetAduSize,
			MP4AV_Rfc3119Concatenator,
			MP4AV_Rfc3119Fragmenter);
	}

	// cleanup
	free(pFrameHeaders);
	pFrameHeaders = NULL;
	free(pAduOffsets);
	pAduOffsets = NULL;

	if (!rc) {
		MP4DeleteTrack(mp4File, hintTrackId);
		return false;
	}

	return true;
}


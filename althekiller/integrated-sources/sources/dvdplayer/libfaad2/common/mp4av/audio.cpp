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

static MP4AV_Mp3Header GetMp3Header(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId)
{
	u_int8_t* pMp3Frame = NULL;
	u_int32_t mp3FrameLength = 0;

	bool rc = MP4ReadSample(
		mp4File,
		audioTrackId,
		1,
		&pMp3Frame,
		&mp3FrameLength);

	if (!rc || mp3FrameLength < 4) {
		return 0;
	}

	MP4AV_Mp3Header mp3Hdr =
		MP4AV_Mp3HeaderFromBytes(pMp3Frame);
	free(pMp3Frame);

	return mp3Hdr;
}

extern "C" u_int8_t MP4AV_AudioGetChannels(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId)
{
	u_int8_t audioType = 
		MP4GetTrackEsdsObjectTypeId(mp4File, audioTrackId);

	if (audioType == MP4_INVALID_AUDIO_TYPE) {
		return 0;
	}

	if (MP4_IS_MP3_AUDIO_TYPE(audioType)) {
		MP4AV_Mp3Header mp3Hdr =
			GetMp3Header(mp4File, audioTrackId);

		if (mp3Hdr == 0) {
			return 0;
		}
		return MP4AV_Mp3GetChannels(mp3Hdr);

	} else if (MP4_IS_AAC_AUDIO_TYPE(audioType)) {
		u_int8_t* pAacConfig = NULL;
		u_int32_t aacConfigLength;

		MP4GetTrackESConfiguration(
			mp4File, 
			audioTrackId,
			&pAacConfig,
			&aacConfigLength);

		if (pAacConfig == NULL || aacConfigLength < 2) {
			return 0;
		}

		u_int8_t channels =
			MP4AV_AacConfigGetChannels(pAacConfig);

		free(pAacConfig);

		return channels;

	} else if ((audioType == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE) ||
	(audioType == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE)) {
		u_int32_t samplesPerFrame =
			MP4GetSampleSize(mp4File, audioTrackId, 1) / 2;

		MP4Duration frameDuration =
			MP4GetSampleDuration(mp4File, audioTrackId, 1);

		if (frameDuration == 0) {
			return 0;
		}

		// assumes track time scale == sampling rate
		return samplesPerFrame / frameDuration;
	}

	return 0;
}

extern "C" u_int32_t MP4AV_AudioGetSamplingRate(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId)
{
	u_int8_t audioType = 
		MP4GetTrackEsdsObjectTypeId(mp4File, audioTrackId);

	if (audioType == MP4_INVALID_AUDIO_TYPE) {
		return 0;
	}

	if (MP4_IS_MP3_AUDIO_TYPE(audioType)) {
		MP4AV_Mp3Header mp3Hdr =
			GetMp3Header(mp4File, audioTrackId);

		if (mp3Hdr == 0) {
			return 0;
		}
		return MP4AV_Mp3GetHdrSamplingRate(mp3Hdr);

	} else if (MP4_IS_AAC_AUDIO_TYPE(audioType)) {
		u_int8_t* pAacConfig = NULL;
		u_int32_t aacConfigLength;

		MP4GetTrackESConfiguration(
			mp4File, 
			audioTrackId,
			&pAacConfig,
			&aacConfigLength);

		if (pAacConfig == NULL || aacConfigLength < 2) {
			return 0;
		}

		u_int32_t samplingRate =
			MP4AV_AacConfigGetSamplingRate(pAacConfig);

		free(pAacConfig);

		return samplingRate;

	} else if ((audioType == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE)||
	(audioType == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE)) {
		return MP4GetTrackTimeScale(mp4File, audioTrackId);
	}

	return 0;
}

extern "C" u_int16_t MP4AV_AudioGetSamplingWindow(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId)
{
	u_int8_t audioType = 
		MP4GetTrackEsdsObjectTypeId(mp4File, audioTrackId);

	if (audioType == MP4_INVALID_AUDIO_TYPE) {
		return 0;
	}

	if (MP4_IS_MP3_AUDIO_TYPE(audioType)) {
		MP4AV_Mp3Header mp3Hdr =
			GetMp3Header(mp4File, audioTrackId);

		return MP4AV_Mp3GetHdrSamplingWindow(mp3Hdr);

	} else if (MP4_IS_AAC_AUDIO_TYPE(audioType)) {
		u_int8_t* pAacConfig = NULL;
		u_int32_t aacConfigLength;

		MP4GetTrackESConfiguration(
			mp4File, 
			audioTrackId,
			&pAacConfig,
			&aacConfigLength);

		if (pAacConfig == NULL || aacConfigLength < 2) {
			return 0;
		}

		u_int32_t samplingWindow =
			MP4AV_AacConfigGetSamplingWindow(pAacConfig);

		free(pAacConfig);

		return samplingWindow;

	} else if ((audioType == MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE)||
	(audioType == MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE)) {
		MP4Duration frameDuration =
			MP4GetSampleDuration(mp4File, audioTrackId, 1);

		// assumes track time scale == sampling rate
		// and constant frame size was used
		return frameDuration;
	}

	return 0;
}

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

#ifndef __MP4AV_ADTS_INCLUDED__
#define __MP4AV_ADTS_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_ADTS_SAMPLING_RATES	16

extern u_int32_t AdtsSamplingRates[NUM_ADTS_SAMPLING_RATES];

bool MP4AV_AdtsGetNextFrame(
	u_int8_t* pSrc, 
	u_int32_t srcLength,
	u_int8_t** ppFrame, 
	u_int32_t* pFrameSize);

u_int16_t MP4AV_AdtsGetFrameSize(
	u_int8_t* pHdr);

u_int16_t MP4AV_AdtsGetHeaderBitSize(
	u_int8_t* pHdr);

u_int16_t MP4AV_AdtsGetHeaderByteSize(
	u_int8_t* pHdr);

u_int8_t MP4AV_AdtsGetVersion(
	u_int8_t* pHdr);

u_int8_t MP4AV_AdtsGetProfile(
	u_int8_t* pHdr);

u_int8_t MP4AV_AdtsGetSamplingRateIndex(
	u_int8_t* pHdr);

u_int8_t MP4AV_AdtsFindSamplingRateIndex(
	u_int32_t samplingRate);

u_int32_t MP4AV_AdtsGetSamplingRate(
	u_int8_t* pHdr);

u_int8_t MP4AV_AdtsGetChannels(
	u_int8_t* pHdr);

bool MP4AV_AdtsMakeFrame(
	u_int8_t* pData,
	u_int16_t dataLength,
	bool isMpeg2,
	u_int8_t profile,
	u_int32_t samplingFrequency,
	u_int8_t channels,
	u_int8_t** ppAdtsData,
	u_int32_t* pAdtsDataLength);

bool MP4AV_AdtsMakeFrameFromMp4Sample(
	MP4FileHandle mp4File,
	MP4TrackId trackId,
	MP4SampleId sampleId,
	int force_profile,
	u_int8_t** ppAdtsData,
	u_int32_t* pAdtsDataLength);

#ifdef __cplusplus
}
#endif
#endif /* __MP4AV_ADTS_INCLUDED__ */

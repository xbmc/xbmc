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

#ifndef __MP4AV_MP3_INCLUDED__
#define __MP4AV_MP3_INCLUDED__

typedef u_int32_t MP4AV_Mp3Header;

#ifdef __cplusplus
extern "C" {
#endif

bool MP4AV_Mp3GetNextFrame(
	const u_int8_t* pSrc, 
	u_int32_t srcLength,
	const u_int8_t** ppFrame, 
	u_int32_t* pFrameSize, 
	bool allowLayer4 DEFAULT_PARM(false),
	bool donthack DEFAULT_PARM(false));

MP4AV_Mp3Header MP4AV_Mp3HeaderFromBytes(const u_int8_t* pBytes);

u_int8_t MP4AV_Mp3GetHdrVersion(MP4AV_Mp3Header hdr);

u_int8_t MP4AV_Mp3GetHdrLayer(MP4AV_Mp3Header hdr);

u_int8_t MP4AV_Mp3GetChannels(MP4AV_Mp3Header hdr);

u_int16_t MP4AV_Mp3GetHdrSamplingRate(MP4AV_Mp3Header hdr);

u_int16_t MP4AV_Mp3GetHdrSamplingWindow(MP4AV_Mp3Header hdr);

u_int16_t MP4AV_Mp3GetSamplingWindow(u_int16_t samplingRate);

u_int16_t MP4AV_Mp3GetBitRate(MP4AV_Mp3Header hdr);

u_int16_t MP4AV_Mp3GetFrameSize(MP4AV_Mp3Header hdr);

u_int16_t MP4AV_Mp3GetAduOffset(const u_int8_t* pFrame, u_int32_t frameSize);

u_int8_t MP4AV_Mp3GetCrcSize(MP4AV_Mp3Header hdr);

u_int8_t MP4AV_Mp3GetSideInfoSize(MP4AV_Mp3Header hdr);

u_int8_t MP4AV_Mp3ToMp4AudioType(u_int8_t mpegVersion);

#ifdef __cplusplus
}
#endif
#endif /* __MP4AV_MP3_INCLUDED__ */

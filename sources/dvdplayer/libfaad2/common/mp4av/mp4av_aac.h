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

#ifndef __MP4AV_AAC_INCLUDED__
#define __MP4AV_AAC_INCLUDED__

#define MP4AV_AAC_MAIN_PROFILE	0
#define MP4AV_AAC_LC_PROFILE	1
#define MP4AV_AAC_SSR_PROFILE	2
#define MP4AV_AAC_LTP_PROFILE	3

#ifdef __cplusplus
extern "C" {
#endif

u_int8_t MP4AV_AacConfigGetSamplingRateIndex(
	u_int8_t* pConfig);

u_int32_t MP4AV_AacConfigGetSamplingRate(
	u_int8_t* pConfig);

u_int16_t MP4AV_AacConfigGetSamplingWindow(
	u_int8_t* pConfig);

u_int8_t MP4AV_AacConfigGetChannels(
	u_int8_t* pConfig);

bool MP4AV_AacGetConfigurationFromAdts(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t* pAdtsHdr);

bool MP4AV_AacGetConfiguration(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t profile,
	u_int32_t samplingRate,
	u_int8_t channels);

#ifdef __cplusplus
}
#endif
#endif /* __MP4AV_AAC_INCLUDED__ */

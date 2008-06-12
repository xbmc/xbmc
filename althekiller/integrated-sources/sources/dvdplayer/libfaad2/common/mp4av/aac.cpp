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
 * AAC Config in ES:
 *
 * AudioObjectType 			5 bits
 * samplingFrequencyIndex 	4 bits
 * if (samplingFrequencyIndex == 0xF)
 *	samplingFrequency	24 bits 
 * channelConfiguration 	4 bits
 * GA_SpecificConfig
 * 	FrameLengthFlag 		1 bit 1024 or 960
 * 	DependsOnCoreCoder		1 bit (always 0)
 * 	ExtensionFlag 			1 bit (always 0)
 */

extern "C" u_int8_t MP4AV_AacConfigGetSamplingRateIndex(u_int8_t* pConfig)
{
	return ((pConfig[0] << 1) | (pConfig[1] >> 7)) & 0xF;
}

extern "C" u_int32_t MP4AV_AacConfigGetSamplingRate(u_int8_t* pConfig)
{
	u_int8_t index =
		MP4AV_AacConfigGetSamplingRateIndex(pConfig);

	if (index == 0xF) {
		return (pConfig[1] & 0x7F) << 17
			| pConfig[2] << 9
			| pConfig[3] << 1
			| (pConfig[4] >> 7);
	}
	return AdtsSamplingRates[index];
}

extern "C" u_int16_t MP4AV_AacConfigGetSamplingWindow(u_int8_t* pConfig)
{
	u_int8_t adjust = 0;

	if (MP4AV_AacConfigGetSamplingRateIndex(pConfig) == 0xF) {
		adjust = 3;
	}

	if ((pConfig[1 + adjust] >> 2) & 0x1) {
		return 960;
	}
	return 1024;
}

extern "C" u_int8_t MP4AV_AacConfigGetChannels(u_int8_t* pConfig)
{
	u_int8_t adjust = 0;

	if (MP4AV_AacConfigGetSamplingRateIndex(pConfig) == 0xF) {
		adjust = 3;
	}
	return (pConfig[1 + adjust] >> 3) & 0xF;
}

extern "C" bool MP4AV_AacGetConfigurationFromAdts(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t* pHdr)
{
	return MP4AV_AacGetConfiguration(
		ppConfig,
		pConfigLength,
		MP4AV_AdtsGetProfile(pHdr),
		MP4AV_AdtsGetSamplingRate(pHdr),
		MP4AV_AdtsGetChannels(pHdr));
}

extern "C" bool MP4AV_AacGetConfiguration(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t profile,
	u_int32_t samplingRate,
	u_int8_t channels)
{
	/* create the appropriate decoder config */

	u_int8_t* pConfig = (u_int8_t*)malloc(2);

	if (pConfig == NULL) {
		return false;
	}

	u_int8_t samplingRateIndex = 
		MP4AV_AdtsFindSamplingRateIndex(samplingRate);

	pConfig[0] =
		((profile + 1) << 3) | ((samplingRateIndex & 0xe) >> 1);
	pConfig[1] =
		((samplingRateIndex & 0x1) << 7) | (channels << 3);

	/* LATER this option is not currently used in MPEG4IP
	if (samplesPerFrame == 960) {
		pConfig[1] |= (1 << 2);
	}
	*/

	*ppConfig = pConfig;
	*pConfigLength = 2;

	return true;
}


extern "C" bool MP4AV_AacGetConfiguration_SBR(
	u_int8_t** ppConfig,
	u_int32_t* pConfigLength,
	u_int8_t profile,
	u_int32_t samplingRate,
	u_int8_t channels)
{
	/* create the appropriate decoder config */

	u_int8_t* pConfig = (u_int8_t*)malloc(5);
    pConfig[0] = 0;
    pConfig[1] = 0;
    pConfig[2] = 0;
    pConfig[3] = 0;
    pConfig[4] = 0;

	if (pConfig == NULL) {
		return false;
	}

	u_int8_t samplingRateIndex = 
		MP4AV_AdtsFindSamplingRateIndex(samplingRate);

	pConfig[0] =
		((profile + 1) << 3) | ((samplingRateIndex & 0xe) >> 1);
	pConfig[1] =
		((samplingRateIndex & 0x1) << 7) | (channels << 3);

    /* pConfig[0] & pConfig[1] now contain the backward compatible
       AudioSpecificConfig
    */

    /* SBR stuff */
    const u_int16_t syncExtensionType = 0x2B7;
	u_int8_t extensionSamplingRateIndex = 
		MP4AV_AdtsFindSamplingRateIndex(2*samplingRate);

    pConfig[2] = (syncExtensionType >> 3) & 0xFF;
    pConfig[3] = ((syncExtensionType & 0x7) << 5) | 5 /* ext ot id */;
    pConfig[4] = ((1 & 0x1) << 7) | (extensionSamplingRateIndex << 3);

	*ppConfig = pConfig;
	*pConfigLength = 5;

	return true;
}

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
#include <mp4av_mpeg4.h>

extern "C" bool MP4AV_Rfc3016Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	u_int16_t maxPayloadSize)
{
	u_int32_t numSamples = MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
	u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
	
	if (numSamples == 0 || maxSampleSize == 0) {
		return false;
	}

	MP4TrackId hintTrackId =
		MP4AddHintTrack(mp4File, mediaTrackId);

	if (hintTrackId == MP4_INVALID_TRACK_ID) {
		return false;
	}

	u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;

	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"MP4V-ES", &payloadNumber, 0);

	/* get the mpeg4 video configuration */
	u_int8_t* pConfig;
	u_int32_t configSize;
	u_int8_t systemsProfileLevel = 0xFE;

	MP4GetTrackESConfiguration(mp4File, mediaTrackId, &pConfig, &configSize);

	if (pConfig) {
		// attempt to get a valid profile-level
		static u_int8_t voshStartCode[4] = { 
			0x00, 0x00, 0x01, MP4AV_MPEG4_VOSH_START 
		};
		if (configSize >= 5 && !memcmp(pConfig, voshStartCode, 4)) {
			systemsProfileLevel = 
				MP4AV_Mpeg4VideoToSystemsProfileLevel(pConfig[4]);
		} 
		if (systemsProfileLevel == 0xFE) {
			u_int8_t iodProfileLevel = MP4GetVideoProfileLevel(mp4File);
			if (iodProfileLevel > 0 && iodProfileLevel < 0xFE) {
				systemsProfileLevel = iodProfileLevel;
			} else {
				systemsProfileLevel = 1;
			}
		} 

		/* convert it into ASCII form */
		char* sConfig = MP4BinaryToBase16(pConfig, configSize);
		if (sConfig == NULL) {
			MP4DeleteTrack(mp4File, hintTrackId);
			return false;
		}

		/* create the appropriate SDP attribute */
		char* sdpBuf = (char*)malloc(strlen(sConfig) + 128);

		sprintf(sdpBuf,
			"a=fmtp:%u profile-level-id=%u; config=%s;\015\012",
				payloadNumber,
				systemsProfileLevel,
				sConfig); 

		/* add this to the track's sdp */
		MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);

		free(sConfig);
		free(sdpBuf);

	}

	u_int8_t* pSampleBuffer = (u_int8_t*)malloc(maxSampleSize);
	if (pSampleBuffer == NULL) {
		MP4DeleteTrack(mp4File, hintTrackId);
		return false;
	}

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
		u_int32_t sampleSize = maxSampleSize;
		MP4Timestamp startTime;
		MP4Duration duration;
		MP4Duration renderingOffset;
		bool isSyncSample;

		bool rc = MP4ReadSample(
			mp4File, mediaTrackId, sampleId, 
			&pSampleBuffer, &sampleSize, 
			&startTime, &duration, 
			&renderingOffset, &isSyncSample);

		if (!rc) {
			MP4DeleteTrack(mp4File, hintTrackId);
			return false;
		}

		bool isBFrame = 
			(MP4AV_Mpeg4GetVopType(pSampleBuffer, sampleSize) == 'B');

		MP4AddRtpVideoHint(mp4File, hintTrackId, isBFrame, renderingOffset);

		if (sampleId == 1) {
			MP4AddRtpESConfigurationPacket(mp4File, hintTrackId);
		}

		u_int32_t offset = 0;
		u_int32_t remaining = sampleSize;

		// TBD should scan for resync markers (if enabled in ES config)
		// and packetize on those boundaries

		while (remaining) {
			bool isLastPacket = false;
			u_int32_t length;

			if (remaining <= maxPayloadSize) {
				length = remaining;
				isLastPacket = true;
			} else {
				length = maxPayloadSize;
			}

			MP4AddRtpPacket(mp4File, hintTrackId, isLastPacket);
			
			MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
				offset, length);

			offset += length;
			remaining -= length;
		}

		MP4WriteRtpHint(mp4File, hintTrackId, duration, isSyncSample);
	}

	return true;
}


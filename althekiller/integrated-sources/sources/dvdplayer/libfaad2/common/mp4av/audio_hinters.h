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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __AUDIO_HINTERS_INCLUDED__
#define __AUDIO_HINTERS_INCLUDED__ 

// Generic Audio Hinters

typedef u_int32_t (*MP4AV_AudioSampleSizer)(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4SampleId sampleId); 

typedef bool (*MP4AV_AudioConcatenator)(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration,
	u_int16_t maxPayloadSize);

typedef bool (*MP4AV_AudioFragmenter)(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t sampleSize, 
	MP4Duration sampleDuration,
	u_int16_t maxPayloadSize);

bool MP4AV_AudioConsecutiveHinter( 
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4Duration sampleDuration, 
	u_int8_t perPacketHeaderSize,
	u_int8_t perSampleHeaderSize,
	u_int8_t maxSamplesPerPacket,
	u_int16_t maxPayloadSize,
	MP4AV_AudioSampleSizer pSizer,
	MP4AV_AudioConcatenator pConcatenator,
	MP4AV_AudioFragmenter pFragmenter);

bool MP4AV_AudioInterleaveHinter( 
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4Duration sampleDuration, 
	u_int8_t stride, 
	u_int8_t bundle,
	u_int16_t maxPayloadSize,
	MP4AV_AudioConcatenator pConcatenator);

MP4Duration MP4AV_GetAudioSampleDuration(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId);

#endif /* __AUDIO_HINTERS_INCLUDED__ */ 


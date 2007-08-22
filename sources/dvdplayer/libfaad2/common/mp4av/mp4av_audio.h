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

#ifndef __MP4AV_AUDIO_INCLUDED__
#define __MP4AV_AUDIO_INCLUDED__ 

// Audio Track Utitlites
#ifdef __cplusplus
extern "C" {
#endif

u_int8_t MP4AV_AudioGetChannels(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId);

u_int32_t MP4AV_AudioGetSamplingRate(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId);

u_int16_t MP4AV_AudioGetSamplingWindow(
	MP4FileHandle mp4File, 
	MP4TrackId audioTrackId);
#ifdef __cplusplus
}
#endif
#endif /* __MP4AV_AUDIO_INCLUDED__ */ 


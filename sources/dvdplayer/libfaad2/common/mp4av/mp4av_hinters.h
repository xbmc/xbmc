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

#ifndef __MP4AV_HINTERS_INCLUDED__
#define __MP4AV_HINTERS_INCLUDED__ 

#define MP4AV_DFLT_PAYLOAD_SIZE		1460

// Audio Hinters

#ifdef __cplusplus
extern "C" {
#endif

bool MP4AV_Rfc2250Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_Rfc3119Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool MP4AV_RfcIsmaHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	bool interleave DEFAULT_PARM(false),
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));


// Video Hinters

bool MP4AV_Rfc3016Hinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	u_int16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool L16Hinter(MP4FileHandle mp4File,
	       MP4TrackId mediaTrackID,
	       uint16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));

bool Mpeg12Hinter(MP4FileHandle mp4File,
		  MP4TrackId mediaTrackID,
		  uint16_t maxPayloadSize DEFAULT_PARM(MP4AV_DFLT_PAYLOAD_SIZE));
#ifdef __cplusplus
}
#endif

#endif /* __MP4AV_HINTERS_INCLUDED__ */ 


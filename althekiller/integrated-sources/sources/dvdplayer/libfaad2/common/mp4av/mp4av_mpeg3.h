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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May (wmay@cisco.com)
 */
#ifndef __MP4AV_MPEG3_H__
#define __MP4AV_MPEG3_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

  int MP4AV_Mpeg3ParseSeqHdr(uint8_t *pbuffer, uint32_t buflen, 
			     int *have_mpeg2,
			      uint32_t *height, uint32_t *width, 
			      double *frame_rate, double *bitrate);

  int MP4AV_Mpeg3PictHdrType(uint8_t *pbuffer);

  int MP4AV_Mpeg3FindGopOrPictHdr(uint8_t *pbuffer, uint32_t buflen, int *ftype);
#ifdef __cplusplus
}
#endif

#endif

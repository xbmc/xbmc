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
 *		Bill May   wmay@cisco.com
 */

#include <mp4av_common.h>

#define DEBUG_L16 1
extern "C" bool L16Hinter (MP4FileHandle mp4file, 
			   MP4TrackId trackid,
			   uint16_t maxPayloadSize)
{
  uint32_t numSamples;
  uint8_t audioType;
  MP4SampleId sampleId;
  uint32_t sampleSize;
  MP4Duration duration;
  char buffer[40];
  MP4TrackId hintTrackId;
  uint8_t payload;
  int chans;
  uint32_t bytes_this_hint;
  uint32_t sampleOffset;

#ifdef DEBUG_L16
  printf("time scale %u\n", MP4GetTrackTimeScale(mp4file, trackid));

  printf("Track fixed sample %llu\n", MP4GetTrackFixedSampleDuration(mp4file, trackid));
#endif

  numSamples = MP4GetTrackNumberOfSamples(mp4file, trackid);

  if (numSamples == 0) return false;


#ifdef DEBUG_L16
  for (unsigned int ix = 1; ix < MIN(10, numSamples); ix++) {
    printf("sampleId %d, size %u duration %llu time %llu\n",
	   ix, MP4GetSampleSize(mp4file, trackid, ix), 
	   MP4GetSampleDuration(mp4file, trackid, ix),
	   MP4GetSampleTime(mp4file, trackid, ix));
  }
#endif

  audioType = MP4GetTrackEsdsObjectTypeId(mp4file, trackid);

  if (audioType != MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE) return false;

  sampleId = 1;
  while ((sampleSize = MP4GetSampleSize(mp4file, trackid, sampleId)) == 0) {
    if (sampleId >= numSamples) return false;
    sampleId++;
  }

  // we have a sampleID with a size.  Give me duration, and we know the
  // number of channels based on the sample size for that duration
  duration = MP4GetSampleDuration(mp4file, trackid, sampleId);

  sampleSize /= sizeof(uint16_t);
  
  if ((sampleSize % duration) != 0) {
#ifdef DEBUG_L16
    printf("Number of samples not correct - duration %llu sample %d\n", 
	   duration, sampleSize);
#endif
    return false;
  }

  chans = sampleSize / duration;
  snprintf(buffer, sizeof(buffer), "%d", chans);

  hintTrackId = MP4AddHintTrack(mp4file, trackid);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  payload = MP4_SET_DYNAMIC_PAYLOAD;
  if (MP4GetTrackTimeScale(mp4file, trackid) == 44100) {
    if (chans == 1) payload = 11;
    else if (chans == 2) payload = 10;
  }
  MP4SetHintTrackRtpPayload(mp4file, hintTrackId, "L16", &payload, 0,
			    chans == 1 ? NULL : buffer);

  sampleId = 1;
  sampleSize = MP4GetSampleSize(mp4file, trackid, sampleId);
  sampleOffset = 0;
  bytes_this_hint = 0;

  if (maxPayloadSize & 0x1) maxPayloadSize--;

  while (1) {
    if (bytes_this_hint == 0) {
#ifdef DEBUG_L16
      printf("Adding hint/packet\n");
#endif
      MP4AddRtpHint(mp4file, hintTrackId);
      MP4AddRtpPacket(mp4file, hintTrackId, false); // marker bit 0
    }
    uint16_t bytes_left_this_packet;
    bytes_left_this_packet = maxPayloadSize - bytes_this_hint;
    if (sampleSize >= bytes_left_this_packet) {
      MP4AddRtpSampleData(mp4file, hintTrackId, 
			  sampleId, sampleOffset, bytes_left_this_packet);
      bytes_this_hint += bytes_left_this_packet;
      sampleSize -= bytes_left_this_packet;
      sampleOffset += bytes_left_this_packet;
#ifdef DEBUG_L16
      printf("Added sample with %d bytes\n", bytes_left_this_packet);
#endif
    } else {
      MP4AddRtpSampleData(mp4file, hintTrackId, 
			  sampleId, sampleOffset, sampleSize);
      bytes_this_hint += sampleSize;
#ifdef DEBUG_L16
      printf("Added sample with %d bytes\n", sampleSize);
#endif
      sampleSize = 0;
    }

    if (bytes_this_hint >= maxPayloadSize) {
      // Write the hint
      // duration is 1/2 of the bytes written
      MP4WriteRtpHint(mp4file, hintTrackId, bytes_this_hint / (2 * chans));
#ifdef DEBUG_L16
      printf("Finished packet - bytes %d\n", bytes_this_hint);
#endif
      bytes_this_hint = 0;
    }
    if (sampleSize == 0) {
      // next sample
      sampleId++;
      if (sampleId > numSamples) {
	// finish it and exit
	if (bytes_this_hint != 0) {
	  MP4WriteRtpHint(mp4file, hintTrackId, bytes_this_hint / 2);
	  return true;
	}
      }
      sampleSize = MP4GetSampleSize(mp4file, trackid, sampleId);
#ifdef DEBUG_L16
      printf("Next sample %d - size %d\n", sampleId, sampleSize);
#endif
      sampleOffset = 0;
    }
  }
	
  return true; // will never reach here
}
  

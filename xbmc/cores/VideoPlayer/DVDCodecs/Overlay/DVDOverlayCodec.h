/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlay.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"

#include <string>

#include "PlatformDefs.h"

// VC_ messages, messages can be combined
enum class OverlayMessage
{
  // an error occurred, no other messages will be returned
  OC_ERROR = 0x00000001,

  // the decoder needs more data
  OC_BUFFER = 0x00000002,

  // the decoder decoded an overlay, call Decode(NULL, 0) again to parse the rest of the data
  OC_OVERLAY = 0x00000004,

  // the decoder has decoded the packet, no overlay will be provided because the previous one is still valid
  OC_DONE = 0x00000008,
};

class CDVDOverlay;
class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

class CDVDOverlayCodec
{
public:

  explicit CDVDOverlayCodec(const char* name) : m_codecName(name) {}

  virtual ~CDVDOverlayCodec() = default;

  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * returns one or a combination of VC_ messages
   * pData and iSize can be NULL, this means we should flush the rest of the data.
   */
  virtual OverlayMessage Decode(DemuxPacket* pPacket) = 0;

  /*
   * Reset the decoder.
   * Should be the same as calling Dispose and Open after each other
   */
  virtual void Reset() = 0;

  /*
   * Flush the current working packet
   * This may leave the internal state intact
   */
  virtual void Flush() = 0;

  /*
   * returns a valid overlay or NULL
   * the data is valid until the next Decode call
   */
  virtual std::shared_ptr<CDVDOverlay> GetOverlay() = 0;

  /*
   * return codecs name
   */
  const std::string& GetName() const { return m_codecName; }

protected:
  /*
   * \brief Adapts startTime, stopTIme from the subtitle stream (which is relative to stream pts)
   * so that it returns the absolute start and stop timestamps.
   */
  static void GetAbsoluteTimes(double& starttime, double& stoptime, DemuxPacket* pkt);

  struct SubtitlePacketExtraData
  {
    double m_chapterStartTime;
  };

private:
  std::string m_codecName;
};

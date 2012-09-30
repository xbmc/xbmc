#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDOverlay.h"
#include "PlatformDefs.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemux.h"

#include <string>

// VC_ messages, messages can be combined
#define OC_ERROR    0x00000001  // an error occured, no other messages will be returned
#define OC_BUFFER   0x00000002  // the decoder needs more data
#define OC_OVERLAY  0x00000004  // the decoder decoded an overlay, call Decode(NULL, 0) again to parse the rest of the data

class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

class CDVDOverlayCodec
{
public:

  CDVDOverlayCodec(const char* name)
  {
    m_codecName = name;
  }

  virtual ~CDVDOverlayCodec() {}

  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;

  /*
   * returns one or a combination of VC_ messages
   * pData and iSize can be NULL, this means we should flush the rest of the data.
   */
  virtual int Decode(DemuxPacket *pPacket) = 0;

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
  virtual CDVDOverlay* GetOverlay() = 0;

  /*
   * return codecs name
   */
  virtual const char* GetName() { return m_codecName.c_str(); }
  
protected:
  /*
   * Adapts startTime, stopTIme from the subtitle stream (which is relative to stream pts)
   * so that it returns the absolute start and stop timestamps.
   * replace - will be set to true if the overlay should replace the former overlay
   * offset - optional - offset will be added to start and stoptime
   */
  static void GetAbsoluteTimes(double &starttime, double &stoptime, DemuxPacket *pkt, bool &replace, double offset = 0.0);

private:
  std::string m_codecName;
};

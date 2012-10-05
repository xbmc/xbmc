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

#pragma once
#include "DVDDemux.h"
#include "filesystem/HTSPSession.h"

class CDVDInputStreamHTSP;
typedef struct htsmsg htsmsg_t;

class CDVDDemuxHTSP : public CDVDDemux
{
public:
  CDVDDemuxHTSP();
  virtual ~CDVDDemuxHTSP();

  bool Open(CDVDInputStream* input);
  void Dispose();
  void Reset();
  void Flush();
  void Abort();
  void SetSpeed(int iSpeed){};

  std::string   GetFileName();

  DemuxPacket*  Read();

  bool          SeekTime(int time, bool backwords = false, double* startpts = NULL) { return false; }
  int           GetStreamLength()                                                   { return 0; }

  CDemuxStream* GetStream(int iStreamId);
  int           GetNrOfStreams();

protected:
  friend class CDemuxStreamVideoHTSP;

  void SubscriptionStart (htsmsg_t *m);
  void SubscriptionStop  (htsmsg_t *m);
  void SubscriptionStatus(htsmsg_t *m);

  htsmsg_t* ReadStream();
  bool      ReadStream(uint8_t* buf, int len);

  typedef std::vector<CDemuxStream*> TStreams;

  CDVDInputStream*     m_Input;
  TStreams             m_Streams;
  std::string          m_Status;
  int                  m_StatusCount;
  HTSP::SQueueStatus   m_QueueStatus;
};

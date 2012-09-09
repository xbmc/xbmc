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

#include "DVDDemux.h"

class CDemuxStreamAudioShoutcast : public CDemuxStreamAudio
{
public:
  virtual void GetStreamInfo(std::string& strInfo);
};

#define SHOUTCAST_BUFFER_SIZE 1024 * 32

class CDVDDemuxShoutcast : public CDVDDemux
{
public:
  CDVDDemuxShoutcast();
  virtual ~CDVDDemuxShoutcast();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset();
  void Flush();
  void Abort(){}
  void SetSpeed(int iSpeed){};
  virtual std::string GetFileName();

  DemuxPacket* Read();

  bool SeekTime(int time, bool backwords = false, double* startpts = NULL);
  int GetStreamLength();
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();

protected:

  CDemuxStreamAudioShoutcast* m_pDemuxStream;

  int m_iMetaStreamInterval;
  CDVDInputStream* m_pInput;
};

/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once
#include "PVRManager.h"
#include "DVDDemux.h"

class CDVDDemuxPVRManager : public CDVDDemux
{
public:
  CDVDDemuxPVRManager();
  virtual ~CDVDDemuxPVRManager();

  bool Open(CDVDInputStream* input);
  void Dispose();
  void Reset();
  void Flush();
  void Abort();
  void SetSpeed(int iSpeed);

  std::string   GetFileName();

  DemuxPacket*  Read();

  bool          SeekTime(int time, bool backwords = false, double* startpts = NULL);
  int           GetStreamLength();

  CDemuxStream* GetStream(int iStreamId);
  int           GetNrOfStreams();

  static bool AddDemuxStream(const PVRDEMUXHANDLE handle, const PVR_DEMUXSTREAMINFO *demux);
  static void DeleteDemuxStream(const PVRDEMUXHANDLE handle, int index);
  static void DeleteDemuxStreams(const PVRDEMUXHANDLE handle);

protected:
  CDVDInputStream  *m_pInput;
  CPVRManager      *m_pManager;
  
  std::vector<CDemuxStream*> m_Streams;
};

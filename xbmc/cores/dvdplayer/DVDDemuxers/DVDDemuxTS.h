/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "DVDDemux.h"
#include "lib/libxdmx/include/tsdemux.h"
#include <map>
#include <deque>


#include <fstream>
class CXdmxInputStream : public IXdmxInputStream
{
public:
  CXdmxInputStream(CDVDInputStream* pInputStream);
  virtual ~CXdmxInputStream();
  virtual unsigned int Read(unsigned char* buf, unsigned int len);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual __int64 GetLength();
  virtual int64_t GetPosition();
  virtual bool IsEOF();
  bool Open(const char* pFilePath);
  CDVDInputStream* GetInnerStream();
protected:
  CDVDInputStream* m_pInputStream;
  __int64 m_Position;
};

class CDVDDemuxTS : public CDVDDemux
{
public:
  CDVDDemuxTS();
  virtual ~CDVDDemuxTS();
  bool Open(CDVDInputStream* pInput, TSTransportType type);
  void Dispose();
  void Reset();
  void Abort();
  void Flush();
  DemuxPacket* Read();
  bool SeekTime(int time, bool backwords = false, double* startpts = NULL);
  void SetSpeed(int iSpeed);

  int GetStreamLength();
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();

  virtual std::string GetFileName();
  void GetStreamCodecName(int iStreamId, CStdString &strName);
protected:
  void AddStream(CElementaryStream* pStream);
  DemuxPacket* GetNextPacket();
  CXdmxInputStream* m_pInput;
  ITransportStreamDemux* m_pInnerDemux;
  CTSProgram* m_pProgram;
  std::vector<CDemuxStream*> m_StreamList;
  std::deque<DemuxPacket*> m_PacketQueue;

  // Debug
  std::map<unsigned short, unsigned __int64> m_StreamCounterList;
  std::map<unsigned short, std::fstream*> m_OutputList;
};
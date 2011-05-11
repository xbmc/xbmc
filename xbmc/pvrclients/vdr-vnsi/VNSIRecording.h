#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include "VNSISession.h"
#include "client.h"

class cVNSIRecording : public cVNSISession
{
public:
  cVNSIRecording();
  ~cVNSIRecording();

  bool OpenRecording(const PVR_RECORDING& recinfo);
  void Close();

  int Read(unsigned char* buf, uint32_t buf_size);
  long long Seek(long long pos, uint32_t whence);
  long long Position(void);
  long long Length(void);

protected:
  void OnReconnect();

private:
  PVR_RECORDING   m_recinfo;
  uint64_t        m_currentPlayingRecordBytes;
  uint32_t        m_currentPlayingRecordFrames;
  uint64_t        m_currentPlayingRecordPosition;

};

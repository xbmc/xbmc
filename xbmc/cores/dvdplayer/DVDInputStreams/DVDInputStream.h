#pragma once

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

#include <string>
#include "utils/BitstreamStats.h"

#include "FileItem.h"

enum DVDStreamType
{
  DVDSTREAM_TYPE_NONE   = -1,
  DVDSTREAM_TYPE_FILE   = 1,
  DVDSTREAM_TYPE_DVD    = 2,
  DVDSTREAM_TYPE_HTTP   = 3,
  DVDSTREAM_TYPE_MEMORY = 4,
  DVDSTREAM_TYPE_FFMPEG = 5,
  DVDSTREAM_TYPE_TV     = 6,
  DVDSTREAM_TYPE_RTMP   = 7,
  DVDSTREAM_TYPE_HTSP   = 8,
  DVDSTREAM_TYPE_MPLS   = 10,
  DVDSTREAM_TYPE_BLURAY = 11,
  DVDSTREAM_TYPE_PVRMANAGER = 12,
};

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

#define DVDSTREAM_BLOCK_SIZE_FILE (2048 * 16)
#define DVDSTREAM_BLOCK_SIZE_DVD  2048

class CDVDInputStream
{
public:
  class IChannel
  {
    public:
    virtual ~IChannel() {};
    virtual bool NextChannel(bool preview = false) = 0;
    virtual bool PrevChannel(bool preview = false) = 0;
    virtual bool SelectChannelByNumber(unsigned int channel) = 0;
    virtual bool SelectChannel(const PVR::CPVRChannel &channel) { return false; };
    virtual bool GetSelectedChannel(PVR::CPVRChannel *) { return false; };
    virtual int GetTotalTime() = 0;
    virtual int GetStartTime() = 0;
    virtual bool UpdateItem(CFileItem& item) = 0;
    virtual bool CanRecord() = 0;
    virtual bool IsRecording() = 0;
    virtual bool Record(bool bOnOff) = 0;
  };

  class IDisplayTime
  {
    public:
    virtual ~IDisplayTime() {};
    virtual int GetTotalTime() = 0;
    virtual int GetTime() = 0;
  };

  class ISeekTime
  {
    public:
    virtual ~ISeekTime() {};
    virtual bool SeekTime(int ms) = 0;
  };

  class IChapter
  {
    public:    
    virtual ~IChapter() {};
    virtual int  GetChapter() = 0;
    virtual int  GetChapterCount() = 0;
    virtual void GetChapterName(std::string& name) = 0;
    virtual bool SeekChapter(int ch) = 0;
  };

  CDVDInputStream(DVDStreamType m_streamType);
  virtual ~CDVDInputStream();
  virtual bool Open(const char* strFileName, const std::string& content);
  virtual void Close() = 0;
  virtual int Read(BYTE* buf, int buf_size) = 0;
  virtual __int64 Seek(__int64 offset, int whence) = 0;
  virtual bool Pause(double dTime) = 0;
  virtual __int64 GetLength() = 0;
  virtual std::string& GetContent() { return m_content; };
  virtual std::string& GetFileName() { return m_strFileName; }
  virtual bool NextStream() { return false; }
  virtual void Abort() {}
  virtual int GetBlockSize() { return 0; }

  /*! \brief Get the number of bytes currently cached/buffered ahead from
   the current position in the input stream if applicable.
   \return number of cached ahead data bytes (-1 if not available)
   */
  virtual __int64 GetCachedBytes() { return -1; }

  /*! \brief Indicate expected read rate in bytes per second.
   *  This could be used to throttle caching rate. Should
   *  be seen as only a hint
   */
  virtual void SetReadRate(unsigned rate) {}

  bool IsStreamType(DVDStreamType type) const { return m_streamType == type; }
  virtual bool IsEOF() = 0;
  virtual int GetCurrentGroupId() { return 0; }
  virtual BitstreamStats GetBitstreamStats() const { return m_stats; }

  void SetFileItem(const CFileItem& item);

protected:
  DVDStreamType m_streamType;
  std::string m_strFileName;
  BitstreamStats m_stats;
  std::string m_content;
  CFileItem m_item;
};

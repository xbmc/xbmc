#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>
#include "utils/BitstreamStats.h"
#include "filesystem/IFileTypes.h"

#include "FileItem.h"
#include "URL.h"
#include "guilib/Geometry.h"

enum DVDStreamType
{
  DVDSTREAM_TYPE_NONE   = -1,
  DVDSTREAM_TYPE_FILE   = 1,
  DVDSTREAM_TYPE_DVD    = 2,
  DVDSTREAM_TYPE_HTTP   = 3,
  DVDSTREAM_TYPE_MEMORY = 4,
  DVDSTREAM_TYPE_FFMPEG = 5,
  DVDSTREAM_TYPE_TV     = 6,
  DVDSTREAM_TYPE_MPLS   = 10,
  DVDSTREAM_TYPE_BLURAY = 11,
  DVDSTREAM_TYPE_PVRMANAGER = 12,
  DVDSTREAM_TYPE_MULTIFILES = 13,
  DVDSTREAM_TYPE_ADDON = 14
};

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

#define DVDSTREAM_BLOCK_SIZE_FILE (2048 * 16)
#define DVDSTREAM_BLOCK_SIZE_DVD  2048

namespace XFILE
{
  class CFile;
}

struct DemuxPacket;
class CDemuxStream;

class CDVDInputStream
{
public:

  class IDisplayTime
  {
    public:
    virtual ~IDisplayTime() {};
    virtual int GetTotalTime() = 0;
    virtual int GetTime() = 0;
  };

  class IPosTime
  {
    public:
    virtual ~IPosTime() {};
    virtual bool PosTime(int ms) = 0;
  };

  class IChapter
  {
    public:
    virtual ~IChapter() {};
    virtual int  GetChapter() = 0;
    virtual int  GetChapterCount() = 0;
    virtual void GetChapterName(std::string& name, int ch=-1) = 0;
    virtual int64_t GetChapterPos(int ch=-1) = 0;
    virtual bool SeekChapter(int ch) = 0;
  };

  class IMenus
  {
    public:
    virtual ~IMenus() {};
    virtual void ActivateButton() = 0;
    virtual void SelectButton(int iButton) = 0;
    virtual int  GetCurrentButton() = 0;
    virtual int  GetTotalButtons() = 0;
    virtual void OnUp() = 0;
    virtual void OnDown() = 0;
    virtual void OnLeft() = 0;
    virtual void OnRight() = 0;
    virtual void OnMenu() = 0;
    virtual void OnBack() = 0;
    virtual void OnNext() = 0;
    virtual void OnPrevious() = 0;
    virtual bool OnMouseMove(const CPoint &point) = 0;
    virtual bool OnMouseClick(const CPoint &point) = 0;
    virtual bool HasMenu() = 0;
    virtual bool IsInMenu() = 0;
    virtual void SkipStill() = 0;
    virtual double GetTimeStampCorrection() { return 0.0; };
    virtual bool GetState(std::string &xmlstate) = 0;
    virtual bool SetState(const std::string &xmlstate) = 0;
  };

  class IDemux
  {
    public:
    virtual ~IDemux() {}
    virtual bool OpenDemux() = 0;
    virtual DemuxPacket* ReadDemux() = 0;
    virtual CDemuxStream* GetStream(int iStreamId) const = 0;
    virtual std::vector<CDemuxStream*> GetStreams() const = 0;
    virtual void EnableStream(int iStreamId, bool enable) = 0;
    virtual int GetNrOfStreams() const = 0;
    virtual void SetSpeed(int iSpeed) = 0;
    virtual bool SeekTime(int time, bool backward = false, double* startpts = NULL) = 0;
    virtual void AbortDemux() = 0;
    virtual void FlushDemux() = 0;
    virtual void SetVideoResolution(int width, int height) {};
  };

  enum ENextStream
  {
    NEXTSTREAM_NONE,
    NEXTSTREAM_OPEN,
    NEXTSTREAM_RETRY,
  };

  CDVDInputStream(DVDStreamType m_streamType, const CFileItem& fileitem);
  virtual ~CDVDInputStream();
  virtual bool Open();
  virtual void Close();
  virtual int Read(uint8_t* buf, int buf_size) = 0;
  virtual int64_t Seek(int64_t offset, int whence) = 0;
  virtual bool Pause(double dTime) = 0;
  virtual int64_t GetLength() = 0;
  virtual std::string& GetContent() { return m_content; };
  virtual std::string GetFileName();
  virtual CURL GetURL();
  virtual ENextStream NextStream() { return NEXTSTREAM_NONE; }
  virtual void Abort() {}
  virtual int GetBlockSize() { return 0; }
  virtual void ResetScanTimeout(unsigned int iTimeoutMs) { }
  virtual bool CanSeek() { return true; }
  virtual bool CanPause() { return true; }

  /*! \brief Indicate expected read rate in bytes per second.
   *  This could be used to throttle caching rate. Should
   *  be seen as only a hint
   */
  virtual void SetReadRate(unsigned rate) {}

  /*! \brief Get the cache status
   \return true when cache status was succesfully obtained
   */
  virtual bool GetCacheStatus(XFILE::SCacheStatus *status) { return false; }

  bool IsStreamType(DVDStreamType type) const { return m_streamType == type; }
  virtual bool IsEOF() = 0;
  virtual BitstreamStats GetBitstreamStats() const { return m_stats; }

  bool ContentLookup() { return m_contentLookup; }

  virtual bool IsRealtime() { return m_realtime; }

  void SetRealtime(bool realtime) { m_realtime = realtime; }

  // interfaces
  virtual IDemux* GetIDemux() { return nullptr; }
  virtual IPosTime* GetIPosTime() { return nullptr; }
  virtual IDisplayTime* GetIDisplayTime() { return nullptr; }

  const CVariant &GetProperty(const std::string key){ return m_item.GetProperty(key); }

protected:
  DVDStreamType m_streamType;
  BitstreamStats m_stats;
  std::string m_content;
  CFileItem m_item;
  bool m_contentLookup;
  bool m_realtime;
};

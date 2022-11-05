/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "URL.h"
#include "cores/MenuType.h"
#include "filesystem/IFileTypes.h"
#include "utils/BitstreamStats.h"
#include "utils/Geometry.h"

#include <string>
#include <vector>

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
    virtual ~IDisplayTime() = default;
    virtual int GetTotalTime() = 0;
    virtual int GetTime() = 0;
  };

  class ITimes
  {
  public:
    struct Times
    {
      time_t startTime;
      double ptsStart;
      double ptsBegin;
      double ptsEnd;
    };
    virtual ~ITimes() = default;
    virtual bool GetTimes(Times &times) = 0;
  };

  class IPosTime
  {
  public:
    virtual ~IPosTime() = default;
    virtual bool PosTime(int ms) = 0;
  };

  class IChapter
  {
  public:
    virtual ~IChapter() = default;
    virtual int  GetChapter() = 0;
    virtual int  GetChapterCount() = 0;
    virtual void GetChapterName(std::string& name, int ch=-1) = 0;
    virtual int64_t GetChapterPos(int ch=-1) = 0;
    virtual bool SeekChapter(int ch) = 0;
  };

  class IMenus
  {
  public:
    virtual ~IMenus() = default;
    virtual void ActivateButton() = 0;
    virtual void SelectButton(int iButton) = 0;
    virtual int  GetCurrentButton() = 0;
    virtual int  GetTotalButtons() = 0;
    virtual void OnUp() = 0;
    virtual void OnDown() = 0;
    virtual void OnLeft() = 0;
    virtual void OnRight() = 0;

    /*! \brief Open the Menu
    * \return true if the menu is successfully opened, false otherwise
    */
    virtual bool OnMenu() = 0;
    virtual void OnBack() = 0;
    virtual void OnNext() = 0;
    virtual void OnPrevious() = 0;
    virtual bool OnMouseMove(const CPoint &point) = 0;
    virtual bool OnMouseClick(const CPoint &point) = 0;

    /*!
    * \brief Get the supported menu type
    * \return The supported menu type
    */
    virtual MenuType GetSupportedMenuType() = 0;

    virtual bool IsInMenu() = 0;
    virtual void SkipStill() = 0;
    virtual double GetTimeStampCorrection() { return 0.0; }
    virtual bool GetState(std::string &xmlstate) = 0;
    virtual bool SetState(const std::string &xmlstate) = 0;
    virtual bool CanSeek() { return !IsInMenu(); }
  };

  class IDemux
  {
  public:
    virtual ~IDemux() = default;
    virtual bool OpenDemux() = 0;
    virtual DemuxPacket* ReadDemux() = 0;
    virtual CDemuxStream* GetStream(int iStreamId) const = 0;
    virtual std::vector<CDemuxStream*> GetStreams() const = 0;
    virtual void EnableStream(int iStreamId, bool enable) {}
    virtual bool OpenStream(int iStreamId) { return false; }
    virtual int GetNrOfStreams() const = 0;
    virtual void SetSpeed(int iSpeed) = 0;
    virtual void FillBuffer(bool mode) {}
    virtual bool SeekTime(double time, bool backward = false, double* startpts = NULL) = 0;
    virtual void AbortDemux() = 0;
    virtual void FlushDemux() = 0;
    virtual void SetVideoResolution(unsigned int width,
                                    unsigned int height,
                                    unsigned int maxWidth,
                                    unsigned int maxHeight)
    {
    }
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
  virtual int64_t GetLength() = 0;
  virtual std::string& GetContent() { return m_content; }
  virtual std::string GetFileName();
  virtual CURL GetURL();
  virtual ENextStream NextStream() { return NEXTSTREAM_NONE; }
  virtual void Abort() {}
  virtual int GetBlockSize() { return 0; }
  virtual bool CanSeek() { return true; } //! @todo drop this
  virtual bool CanPause() { return false; }

  /*! \brief Indicate expected read rate in bytes per second.
   *  This could be used to throttle caching rate. Should
   *  be seen as only a hint
   */
  virtual void SetReadRate(uint32_t rate) {}

  /*! \brief Get the cache status
   \return true when cache status was successfully obtained
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
  virtual ITimes* GetITimes() { return nullptr; }
  virtual IChapter* GetIChapter() { return nullptr; }

  const CVariant& GetProperty(const std::string& key) { return m_item.GetProperty(key); }

protected:
  DVDStreamType m_streamType;
  BitstreamStats m_stats;
  std::string m_content;
  CFileItem m_item;
  bool m_contentLookup;
  bool m_realtime;
};

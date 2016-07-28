/*
 *      Copyright (C) 2005-2016 Team XBMC
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

#pragma once

#include <memory>
#include <vector>

#include "DVDInputStream.h"
#include "addons/InputStream.h"

//! \brief Input stream class
class CInputStreamAddon :
  public CDVDInputStream,
  public CDVDInputStream::IDisplayTime,
  public CDVDInputStream::IPosTime,
  public CDVDInputStream::IDemux
{
public:
  //! \brief constructor
  CInputStreamAddon(const CFileItem& fileitem, std::shared_ptr<ADDON::CInputStream> inputStream);

  //! \brief Destructor.
  virtual ~CInputStreamAddon();

  //! \brief Open a MPD file
  virtual bool Open() override;

  //! \brief Close input stream
  virtual void Close() override;

  //! \brief Read data from stream
  virtual int Read(uint8_t* buf, int buf_size) override;

  //! \brief Seeek in stream
  virtual int64_t Seek(int64_t offset, int whence) override;

  //! \brief Pause stream
  virtual bool Pause(double dTime) override;
  //! \brief Return true if we have reached EOF
  virtual bool IsEOF() override;

  virtual bool CanSeek() override;
  virtual bool CanPause() override;

  //! \brief Get length of input data
  virtual int64_t GetLength() override;

  // IDisplayTime
  virtual CDVDInputStream::IDisplayTime* GetIDisplayTime() override;
  virtual int GetTotalTime() override;
  virtual int GetTime() override;

  // IPosTime
  virtual CDVDInputStream::IPosTime* GetIPosTime() override;
  virtual bool PosTime(int ms) override;

  //IDemux
  CDVDInputStream::IDemux* GetIDemux() override;
  virtual bool OpenDemux() override;
  virtual DemuxPacket* ReadDemux() override;
  virtual CDemuxStream* GetStream(int iStreamId) const override;
  virtual std::vector<CDemuxStream*> GetStreams() const override;
  virtual void EnableStream(int iStreamId, bool enable) override;
  virtual int GetNrOfStreams() const override;
  virtual void SetSpeed(int iSpeed) override;
  virtual bool SeekTime(int time, bool backward = false, double* startpts = NULL) override;
  virtual void AbortDemux() override;
  virtual void FlushDemux() override;
  virtual void SetVideoResolution(int width, int height) override;

protected:
  std::shared_ptr<ADDON::CInputStream> m_addon;
  bool m_hasDemux;
  bool m_hasDisplayTime;
  bool m_hasPosTime;
  bool m_canPause;
  bool m_canSeek;
};

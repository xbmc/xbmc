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
#include "IVideoPlayer.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Inputstream.h"

//! \brief Input stream class
class CInputStreamAddon
  : public ADDON::IAddonInstanceHandler,
    public CDVDInputStream,
    public CDVDInputStream::IDisplayTime,
    public CDVDInputStream::IPosTime,
    public CDVDInputStream::IDemux
{
public:
  CInputStreamAddon(ADDON::BinaryAddonBasePtr& addonBase, IVideoPlayer* player, const CFileItem& fileitem);
  virtual ~CInputStreamAddon();

  static bool Supports(ADDON::BinaryAddonBasePtr& addonBase, const CFileItem& fileitem);

  // CDVDInputStream
  virtual bool Open() override;
  virtual void Close() override;
  virtual int Read(uint8_t* buf, int buf_size) override;
  virtual int64_t Seek(int64_t offset, int whence) override;
  virtual bool Pause(double dTime) override;
  virtual int64_t GetLength() override;
  virtual bool IsEOF() override;
  virtual bool CanSeek() override;
  virtual bool CanPause() override;

  // IDisplayTime
  virtual CDVDInputStream::IDisplayTime* GetIDisplayTime() override;
  virtual int GetTotalTime() override;
  virtual int GetTime() override;

  // IPosTime
  virtual CDVDInputStream::IPosTime* GetIPosTime() override;
  virtual bool PosTime(int ms) override;

  // IDemux
  CDVDInputStream::IDemux* GetIDemux() override;
  virtual bool OpenDemux() override;
  virtual DemuxPacket* ReadDemux() override;
  virtual CDemuxStream* GetStream(int streamId) const override;
  virtual std::vector<CDemuxStream*> GetStreams() const override;
  virtual void EnableStream(int streamId, bool enable) override;
  virtual int GetNrOfStreams() const override;
  virtual void SetSpeed(int speed) override;
  virtual bool SeekTime(double time, bool backward = false, double* startpts = nullptr) override;
  virtual void AbortDemux() override;
  virtual void FlushDemux() override;
  virtual void SetVideoResolution(int width, int height) override;
  int64_t PositionStream();
  bool IsRealTimeStream();

protected:
  void UpdateStreams();
  void DisposeStreams();

  IVideoPlayer* m_player;

private:
  std::vector<std::string> m_fileItemProps;
  INPUTSTREAM_CAPABILITIES m_caps;
  std::map<int, CDemuxStream*> m_streams;

  AddonInstance_InputStream m_struct;

  /*!
   * Callbacks from add-on to kodi
   */
  //@{
  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param kodiInstance A pointer to the add-on.
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet.
   */
  static DemuxPacket* cb_allocate_demux_packet(void* kodiInstance, int iDataSize = 0);

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param kodiInstance A pointer to the add-on.
   * @param pPacket The packet to free.
   */
  static void cb_free_demux_packet(void* kodiInstance, DemuxPacket* pPacket);
  //@}
};

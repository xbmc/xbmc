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
#include "addons/AddonDll.h"
#include "addons/AddonProvider.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Inputstream.h"

class CInputStreamProvider
  : public ADDON::CAddonProvider
{
public:
  CInputStreamProvider(ADDON::AddonInfoPtr addonInfo, kodi::addon::IAddonInstance* parentInstance);

  virtual void getAddonInstance(INSTANCE_TYPE instance_type, ADDON::AddonInfoPtr& addonInfo, kodi::addon::IAddonInstance*& parentInstance);

private:
  ADDON::AddonInfoPtr m_addonInfo;
  kodi::addon::IAddonInstance* m_parentInstance;
};

//! \brief Input stream class
class CInputStreamAddon :
  public CDVDInputStream,
  public CDVDInputStream::IDisplayTime,
  public CDVDInputStream::IPosTime,
  public CDVDInputStream::IDemux,
  public ADDON::IAddonInstanceHandler
{
public:
  //! \brief constructor
  CInputStreamAddon(ADDON::AddonInfoPtr addonInfo, IVideoPlayer* player, const CFileItem& fileitem);

  static bool Supports(ADDON::AddonInfoPtr& addonInfo, const CFileItem& fileitem);

  //! \brief Destructor.
  virtual ~CInputStreamAddon();

  //! \brief Open a MPD file
  virtual bool Open() override;

  //! \brief Close input stream
  virtual void Close() override;

  //! \brief Read data from stream
  virtual int Read(uint8_t* buf, int buf_size) override;

  //! \brief Seek in stream
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
  virtual bool SeekTime(double time, bool backward = false, double* startpts = NULL) override;
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

  ADDON::AddonInfoPtr m_addonInfo;
  ADDON::AddonDllPtr m_addon;
  kodi::addon::CInstanceInputStream* m_addonInstance;
  AddonInstance_InputStream m_struct;
  std::shared_ptr<CInputStreamProvider> m_subAddonProvider;

  /*!
   * Callbacks from add-on to kodi
   */
  //@{
  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param kodiInstanceBase A pointer to this.
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet.
   */
  static DemuxPacket* InputStreamAllocateDemuxPacket(void* kodiInstanceBase, int iDataSize = 0);

  /*!
   * @brief Allocate a demux packet with crypto data. Free with FreeDemuxPacket
   * @param kodiInstanceBase A pointer to this.
   * @param iDataSize The size of the data that will go into the packet
   * @param encryptedSubsampleCount The number of encrypted subSamples that will go into the packet
   * @return The allocated packet.
   */
  static DemuxPacket* InputStreamAllocateEncryptedDemuxPacket(void* kodiInstanceBase, unsigned int iDataSize, unsigned int encryptedSubsampleCount);

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param kodiInstanceBase A pointer to this.
   * @param pPacket The packet to free.
   */
  static void InputStreamFreeDemuxPacket(void* kodiInstanceBase, DemuxPacket* pPacket);
  //@}
};

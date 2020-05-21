/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDInputStream.h"
#include "IVideoPlayer.h"
#include "addons/AddonProvider.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Inputstream.h"

#include <memory>
#include <vector>

class CInputStreamProvider
  : public ADDON::IAddonProvider
{
public:
  CInputStreamProvider(ADDON::BinaryAddonBasePtr addonBase, KODI_HANDLE parentInstance);

  void getAddonInstance(INSTANCE_TYPE instance_type,
                        ADDON::BinaryAddonBasePtr& addonBase,
                        KODI_HANDLE& parentInstance) override;

private:
  ADDON::BinaryAddonBasePtr m_addonBase;
  KODI_HANDLE m_parentInstance;
};

//! \brief Input stream class
class CInputStreamAddon
  : public ADDON::IAddonInstanceHandler
  , public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::ITimes
  , public CDVDInputStream::IPosTime
  , public CDVDInputStream::IDemux
  , public CDVDInputStream::IChapter
{
public:
  CInputStreamAddon(ADDON::BinaryAddonBasePtr& addonBase, IVideoPlayer* player, const CFileItem& fileitem);
  ~CInputStreamAddon() override;

  static bool Supports(ADDON::BinaryAddonBasePtr& addonBase, const CFileItem& fileitem);

  // CDVDInputStream
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  int64_t GetLength() override;
  int GetBlockSize() override;
  bool IsEOF() override;
  bool CanSeek() override; //! @todo drop this
  bool CanPause() override;

  // IDisplayTime
  CDVDInputStream::IDisplayTime* GetIDisplayTime() override;
  int GetTotalTime() override;
  int GetTime() override;

  // ITime
  CDVDInputStream::ITimes* GetITimes() override;
  bool GetTimes(Times &times) override;

  // IPosTime
  CDVDInputStream::IPosTime* GetIPosTime() override;
  bool PosTime(int ms) override;

  // IDemux
  CDVDInputStream::IDemux* GetIDemux() override;
  bool OpenDemux() override;
  DemuxPacket* ReadDemux() override;
  CDemuxStream* GetStream(int streamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  void EnableStream(int streamId, bool enable) override;
  bool OpenStream(int streamid) override;

  int GetNrOfStreams() const override;
  void SetSpeed(int speed) override;
  bool SeekTime(double time, bool backward = false, double* startpts = nullptr) override;
  void AbortDemux() override;
  void FlushDemux() override;
  void SetVideoResolution(int width, int height) override;
  bool IsRealtime() override;

  // IChapter
  CDVDInputStream::IChapter* GetIChapter() override;
  int GetChapter() override;
  int GetChapterCount() override;
  void GetChapterName(std::string& name, int ch = -1) override;
  int64_t GetChapterPos(int ch = -1) override;
  bool SeekChapter(int ch) override;

protected:
  static int ConvertVideoCodecProfile(STREAMCODEC_PROFILE profile);

  IVideoPlayer* m_player;

private:
  std::vector<std::string> m_fileItemProps;
  INPUTSTREAM_CAPABILITIES m_caps;

  int m_streamCount = 0;

  AddonInstance_InputStream m_struct;
  std::shared_ptr<CInputStreamProvider> m_subAddonProvider;

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
  * @brief Allocate an encrypted demux packet. Free with FreeDemuxPacket
  * @param kodiInstance A pointer to the add-on.
  * @param dataSize The size of the data that will go into the packet
  * @param encryptedSubsampleCount The number of subsample description blocks to allocate
  * @return The allocated packet.
  */
  static DemuxPacket* cb_allocate_encrypted_demux_packet(void* kodiInstance, unsigned int dataSize, unsigned int encryptedSubsampleCount);

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param kodiInstance A pointer to the add-on.
   * @param pPacket The packet to free.
   */
  static void cb_free_demux_packet(void* kodiInstance, DemuxPacket* pPacket);
  //@}
};

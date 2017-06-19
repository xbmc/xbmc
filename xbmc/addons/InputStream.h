/*
 *      Copyright (C) 2016 Team Kodi
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

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_inputstream_types.h"
#include "FileItem.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "threads/CriticalSection.h"
#include <vector>
#include <map>

class CDemuxStream;

namespace ADDON
{

  class CInputStream : public CAddonDll
  {
  public:

    static std::unique_ptr<CInputStream> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CInputStream(CAddonInfo addonInfo)
      : CAddonDll(std::move(addonInfo))
    {};
    CInputStream(const CAddonInfo& addonInfo,
                 const std::string& name,
                 const std::string& listitemprops,
                 const std::string& extensions,
                 const std::string& protocols);
    virtual ~CInputStream() {}

    virtual void SaveSettings() override;

    bool Create();

    bool UseParent();
    bool Supports(const CFileItem &fileitem);
    bool Open(CFileItem &fileitem);
    void Close();

    bool HasDemux() { return m_caps.m_supportsIDemux; };
    bool HasPosTime() { return m_caps.m_supportsIPosTime; };
    bool HasDisplayTime() { return m_caps.m_supportsIDisplayTime; };
    bool CanPause() { return m_caps.m_supportsPause; };
    bool CanSeek() { return m_caps.m_supportsSeek; };

    // IDisplayTime
    int GetTotalTime();
    int GetTime();

    // IPosTime
    bool PosTime(int ms);

    // demux
    int GetNrOfStreams() const;
    CDemuxStream* GetStream(int iStreamId);
    std::vector<CDemuxStream*> GetStreams() const;
    DemuxPacket* ReadDemux();
    bool SeekTime(double time, bool backward, double* startpts);
    void AbortDemux();
    void FlushDemux();
    void SetSpeed(int iSpeed);
    void EnableStream(int iStreamId, bool enable);
    void SetVideoResolution(int width, int height);

    // stream
    int ReadStream(uint8_t* buf, unsigned int size);
    int64_t SeekStream(int64_t offset, int whence);
    int64_t PositionStream();
    int64_t LengthStream();
    void PauseStream(double time);
    bool IsRealTimeStream();

    /*!
     * @brief To get the interface table used between addon and kodi
     * @todo This function becomes removed after old callback library system
     * is removed.
     */
    AddonInstance_InputStream* GetInstanceInterface() { return &m_struct; }

  protected:
    void UpdateStreams();
    void DisposeStreams();
    void UpdateConfig();
    void CheckConfig();

    std::vector<std::string> m_fileItemProps;
    std::vector<std::string> m_extensionsList;
    std::vector<std::string> m_protocolsList;
    INPUTSTREAM_CAPABILITIES m_caps;
    std::map<int, CDemuxStream*> m_streams;

    static CCriticalSection m_parentSection;

    struct Config
    {
      std::vector<std::string> m_pathList;
      bool m_parentBusy;
      bool m_ready;
    };
    static std::map<std::string, Config> m_configMap;

  private:
    /*!
     * @brief Callback functions from addon to kodi
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

    AddonInstance_InputStream m_struct;
  };

} /*namespace ADDON*/

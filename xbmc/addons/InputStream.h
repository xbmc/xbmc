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

#include "AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_inputstream_types.h"
#include "FileItem.h"
#include "threads/CriticalSection.h"
#include <vector>
#include <map>

typedef DllAddon<InputStreamAddonFunctions, INPUTSTREAM_PROPS> DllInputStream;

class CDemuxStream;

namespace ADDON
{
  typedef CAddonDll<DllInputStream, InputStreamAddonFunctions, INPUTSTREAM_PROPS> InputStreamDll;

  class CInputStream : public InputStreamDll
  {
  public:

    static std::unique_ptr<CInputStream> FromExtension(AddonProps props, const cp_extension_t* ext);

    explicit CInputStream(AddonProps props)
      : InputStreamDll(std::move(props))
    {};
    CInputStream(AddonProps props, std::string name, std::string listitemprops, std::string extensions);
    virtual ~CInputStream() {}

    virtual void SaveSettings() override;

    bool UseParent();
    bool Supports(const CFileItem &fileitem);
    bool Open(CFileItem &fileitem);
    void Close();

    bool HasDemux() { return m_caps.m_supportsIDemux; };
    bool HasPosTime() { return m_caps.m_supportsIPosTime; };
    bool HasDisplayTime() { return m_caps.m_supportsIDisplayTime; };
    bool CanPause() { return m_caps.m_supportsPause; };
    bool CanSeek() { return m_caps.m_supportsSeek; };
    bool CanEnableAtPTS() { return m_caps.m_supportsEnableAtPTS; };

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
    bool SeekTime(int time, bool backward, double* startpts);
    void AbortDemux();
    void FlushDemux();
    void SetSpeed(int iSpeed);
    void EnableStream(int iStreamId, bool enable);
    void EnableStreamAtPTS(int iStreamId, uint64_t pts);
    void SetVideoResolution(int width, int height);

    // stream
    int ReadStream(uint8_t* buf, unsigned int size);
    int64_t SeekStream(int64_t offset, int whence);
    int64_t PositionStream();
    int64_t LengthStream();
    void PauseStream(double time);
    bool IsRealTimeStream();

  protected:
    void UpdateStreams();
    void DisposeStreams();
    void UpdateConfig();

    std::vector<std::string> m_fileItemProps;
    std::vector<std::string> m_extensionsList;
    INPUTSTREAM_CAPABILITIES m_caps;
    std::map<int, CDemuxStream*> m_streams;

    static CCriticalSection m_parentSection;

    struct Config
    {
      std::vector<std::string> m_pathList;
      bool m_parentBusy;
    };
    static std::map<std::string, Config> m_configMap;
  };

} /*namespace ADDON*/

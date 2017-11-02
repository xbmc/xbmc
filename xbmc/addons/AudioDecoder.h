/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/AudioDecoder.h"
#include "cores/paplayer/ICodec.h"
#include "music/tags/ImusicInfoTagLoader.h"
#include "filesystem/MusicFileDirectory.h"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
  class EmbeddedArt;
}

namespace ADDON
{

  class CAudioDecoder : public IAddonInstanceHandler,
                        public ICodec,
                        public MUSIC_INFO::IMusicInfoTagLoader,
                        public XFILE::CMusicFileDirectory
  {
  public:
    explicit CAudioDecoder(const BinaryAddonBasePtr& addonInfo);
    ~CAudioDecoder() override;

    // Things that MUST be supplied by the child classes
    bool CreateDecoder();
    bool Init(const CFileItem& file, unsigned int filecache) override;
    int ReadPCM(uint8_t* buffer, int size, int* actualsize) override;
    bool Seek(int64_t time) override;
    bool CanInit() override { return true; }
    bool Load(const std::string& strFileName,
                      MUSIC_INFO::CMusicInfoTag& tag,
                      MUSIC_INFO::EmbeddedArt *art = nullptr) override;
    int GetTrackCount(const std::string& strPath) override;

    static inline std::string GetExtensions(const BinaryAddonBasePtr& addonInfo)
    {
      return addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@extension").asString();
    }

    static inline std::string GetMimetypes(const BinaryAddonBasePtr& addonInfo)
    {
      return addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@mimetype").asString();
    }

    static inline bool HasTags(const BinaryAddonBasePtr& addonInfo)
    {
      return addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@tags").asBoolean();
    }

    static inline bool HasTracks(const BinaryAddonBasePtr& addonInfo)
    {
      return addonInfo->Type(ADDON_AUDIODECODER)->GetValue("@tracks").asBoolean();
    }

  private:
    const AEChannel* m_channel;
    AddonInstance_AudioDecoder m_struct;
    bool m_hasTags;
  };

} /*namespace ADDON*/

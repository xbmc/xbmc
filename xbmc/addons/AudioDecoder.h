/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
                      EmbeddedArt *art = nullptr) override;
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

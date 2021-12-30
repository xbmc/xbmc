/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddonSupportCheck.h"
#include "addons/IAddonSupportList.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/AudioDecoder.h"
#include "cores/paplayer/ICodec.h"
#include "filesystem/MusicFileDirectory.h"
#include "music/tags/ImusicInfoTagLoader.h"

namespace KODI
{
namespace ADDONS
{

class CAudioDecoder : public ADDON::IAddonInstanceHandler,
                      public IAddonSupportCheck,
                      public ICodec,
                      public MUSIC_INFO::IMusicInfoTagLoader,
                      public XFILE::CMusicFileDirectory
{
public:
  explicit CAudioDecoder(const ADDON::AddonInfoPtr& addonInfo);
  ~CAudioDecoder() override;

  // Things that MUST be supplied by the child classes
  bool CreateDecoder();
  bool Init(const CFileItem& file, unsigned int filecache) override;
  int ReadPCM(uint8_t* buffer, size_t size, size_t* actualsize) override;
  bool Seek(int64_t time) override;
  bool CanInit() override { return true; }
  bool Load(const std::string& strFileName,
            MUSIC_INFO::CMusicInfoTag& tag,
            EmbeddedArt* art = nullptr) override;
  int GetTrackCount(const std::string& strPath) override;
  bool SupportsFile(const std::string& filename) override;

private:
  bool m_hasTags;
};

} /* namespace ADDONS */
} /* namespace KODI */

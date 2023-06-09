/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureBundleXBT.h"

#include "ServiceBroker.h"
#include "Texture.h"
#include "URL.h"
#include "XBTF.h"
#include "XBTFReader.h"
#include "commons/ilog.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/XbtManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cstdint>
#include <exception>

#include <lzo/lzo1x.h>
#include <lzo/lzoconf.h>


#ifdef TARGET_WINDOWS_DESKTOP
#ifdef NDEBUG
#pragma comment(lib,"lzo2.lib")
#else
#pragma comment(lib, "lzo2d.lib")
#endif
#endif

CTextureBundleXBT::CTextureBundleXBT()
  : m_TimeStamp{0}
  , m_themeBundle{false}
{
}

CTextureBundleXBT::CTextureBundleXBT(bool themeBundle)
  : m_TimeStamp{0}
  , m_themeBundle{themeBundle}
{
}

CTextureBundleXBT::~CTextureBundleXBT(void)
{
  CloseBundle();
}

void CTextureBundleXBT::CloseBundle()
{
  if (m_XBTFReader != nullptr && m_XBTFReader->IsOpen())
  {
    XFILE::CXbtManager::GetInstance().Release(CURL(m_path));
    CLog::Log(LOGDEBUG, "{} - Closed {}bundle", __FUNCTION__, m_themeBundle ? "theme " : "");
  }
}

bool CTextureBundleXBT::OpenBundle()
{
  // Find the correct texture file (skin or theme)
  if (m_themeBundle)
  {
    // if we are the theme bundle, we only load if the user has chosen
    // a valid theme (or the skin has a default one)
    std::string theme = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
    if (!theme.empty() && !StringUtils::EqualsNoCase(theme, "SKINDEFAULT"))
    {
      std::string themeXBT(URIUtils::ReplaceExtension(theme, ".xbt"));
      m_path = URIUtils::AddFileToFolder(CServiceBroker::GetWinSystem()->GetGfxContext().GetMediaDir(), "media", themeXBT);
    }
    else
    {
      return false;
    }
  }
  else
  {
    m_path = URIUtils::AddFileToFolder(CServiceBroker::GetWinSystem()->GetGfxContext().GetMediaDir(), "media", "Textures.xbt");
  }

  m_path = CSpecialProtocol::TranslatePathConvertCase(m_path);

  // Load the texture file
  if (!XFILE::CXbtManager::GetInstance().GetReader(CURL(m_path), m_XBTFReader))
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "{} - Opened bundle {}", __FUNCTION__, m_path);

  m_TimeStamp = m_XBTFReader->GetLastModificationTimestamp();

  if (lzo_init() != LZO_E_OK)
  {
    return false;
  }

  return true;
}

bool CTextureBundleXBT::HasFile(const std::string& Filename)
{
  if ((m_XBTFReader == nullptr || !m_XBTFReader->IsOpen()) && !OpenBundle())
    return false;

  if (m_XBTFReader->GetLastModificationTimestamp() > m_TimeStamp)
  {
    CLog::Log(LOGINFO, "Texture bundle has changed, reloading");
    if (!OpenBundle())
      return false;
  }

  std::string name = Normalize(Filename);
  return m_XBTFReader->Exists(name);
}

std::vector<std::string> CTextureBundleXBT::GetTexturesFromPath(const std::string& path)
{
  if (path.size() > 1 && path[1] == ':')
    return {};

  if ((m_XBTFReader == nullptr || !m_XBTFReader->IsOpen()) && !OpenBundle())
    return {};

  std::string testPath = Normalize(path);
  URIUtils::AddSlashAtEnd(testPath);

  std::vector<std::string> textures;
  std::vector<CXBTFFile> files = m_XBTFReader->GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    std::string filePath = files[i].GetPath();
    if (StringUtils::StartsWithNoCase(filePath, testPath))
      textures.emplace_back(std::move(filePath));
  }

  return textures;
}

std::optional<CTextureBundleXBT::Texture> CTextureBundleXBT::LoadTexture(
    const std::string& filename)
{
  std::string name = Normalize(filename);

  CXBTFFile file;
  if (!m_XBTFReader->Get(name, file))
    return {};

  if (file.GetFrames().empty())
    return {};

  const CXBTFFrame& frame = file.GetFrames().at(0);

  Texture texture;
  texture.width = frame.GetWidth();
  texture.height = frame.GetHeight();

  texture.texture = ConvertFrameToTexture(filename, frame);
  if (!texture.texture)
    return {};

  return std::make_optional<Texture>(std::move(texture));
}

std::optional<CTextureBundleXBT::Animation> CTextureBundleXBT::LoadAnim(const std::string& filename)
{
  std::string name = Normalize(filename);

  CXBTFFile file;
  if (!m_XBTFReader->Get(name, file))
    return {};

  if (file.GetFrames().empty())
    return {};

  size_t nTextures = file.GetFrames().size();

  Animation animation;
  animation.textures.reserve(nTextures);

  for (size_t i = 0; i < nTextures; i++)
  {
    CXBTFFrame& frame = file.GetFrames().at(i);

    std::unique_ptr<CTexture> texture = ConvertFrameToTexture(filename, frame);
    if (!texture)
      return {};

    animation.textures.emplace_back(std::move(texture), frame.GetDuration());
  }

  animation.width = file.GetFrames().at(0).GetWidth();
  animation.height = file.GetFrames().at(0).GetHeight();
  animation.loops = file.GetLoop();

  return std::make_optional<Animation>(std::move(animation));
}

std::unique_ptr<CTexture> CTextureBundleXBT::ConvertFrameToTexture(const std::string& name,
                                                                   const CXBTFFrame& frame)
{
  // found texture - allocate the necessary buffers
  std::vector<unsigned char> buffer(static_cast<size_t>(frame.GetPackedSize()));

  // load the compressed texture
  if (!m_XBTFReader->Load(frame, buffer.data()))
  {
    CLog::Log(LOGERROR, "Error loading texture: {}", name);
    return {};
  }

  // check if it's packed with lzo
  if (frame.IsPacked())
  { // unpack
    std::vector<unsigned char> unpacked(static_cast<size_t>(frame.GetUnpackedSize()));
    lzo_uint s = (lzo_uint)frame.GetUnpackedSize();
    if (lzo1x_decompress_safe(buffer.data(), static_cast<lzo_uint>(buffer.size()), unpacked.data(),
                              &s, NULL) != LZO_E_OK ||
        s != frame.GetUnpackedSize())
    {
      CLog::Log(LOGERROR, "Error loading texture: {}: Decompression error", name);
      return {};
    }
    buffer = std::move(unpacked);
  }

  // create an xbmc texture
  std::unique_ptr<CTexture> texture = CTexture::CreateTexture();
  texture->LoadFromMemory(frame.GetWidth(), frame.GetHeight(), 0, frame.GetFormat(),
                          frame.HasAlpha(), buffer.data());

  return texture;
}

void CTextureBundleXBT::SetThemeBundle(bool themeBundle)
{
  m_themeBundle = themeBundle;
}

// normalize to how it's stored within the bundle
// lower case + using forward slash rather than back slash
std::string CTextureBundleXBT::Normalize(std::string name)
{
  StringUtils::Trim(name);
  StringUtils::ToLower(name);
  StringUtils::Replace(name, '\\', '/');

  return name;
}

std::vector<uint8_t> CTextureBundleXBT::UnpackFrame(const CXBTFReader& reader,
                                                    const CXBTFFrame& frame)
{
  // load the compressed texture
  std::vector<uint8_t> packedBuffer(static_cast<size_t>(frame.GetPackedSize()));
  if (!reader.Load(frame, packedBuffer.data()))
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: error loading frame");
    return {};
  }

  // if the frame isn't packed there's nothing else to be done
  if (!frame.IsPacked())
    return packedBuffer;

  // make sure lzo is initialized
  if (lzo_init() != LZO_E_OK)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: failed to initialize lzo");
    return {};
  }

  lzo_uint size = static_cast<lzo_uint>(frame.GetUnpackedSize());
  std::vector<uint8_t> unpackedBuffer(static_cast<size_t>(frame.GetUnpackedSize()));
  if (lzo1x_decompress_safe(packedBuffer.data(), static_cast<lzo_uint>(packedBuffer.size()),
                            unpackedBuffer.data(), &size, nullptr) != LZO_E_OK ||
      size != frame.GetUnpackedSize())
  {
    CLog::Log(LOGERROR,
              "CTextureBundleXBT: failed to decompress frame with {} unpacked bytes to {} bytes",
              frame.GetPackedSize(), frame.GetUnpackedSize());
    return {};
  }

  return unpackedBuffer;
}

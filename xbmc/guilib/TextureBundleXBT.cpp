/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TextureBundleXBT.h"

#include "system.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "XBTF.h"
#include "XBTFReader.h"
#include <lzo/lzo1x.h>

#ifdef TARGET_WINDOWS
#ifdef NDEBUG
#pragma comment(lib,"lzo2.lib")
#else
#pragma comment(lib, "lzo2-no_idb.lib")
#endif
#endif

bool CTextureBundleXBT::Open()
{
  // Find the correct texture file (skin or theme)
  auto mediaDir = g_graphicsContext.GetMediaDir();
  if (mediaDir.empty())
  {
    mediaDir = CSpecialProtocol::TranslatePath(
      URIUtils::AddFileToFolder("special://home/addons",
        CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKIN)));
  }

  if (m_themeBundle)
  {
    // if we are the theme bundle, we only load if the user has chosen
    // a valid theme (or the skin has a default one)
    std::string theme = CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
    if (!theme.empty() && !StringUtils::EqualsNoCase(theme, "SKINDEFAULT"))
    {
      std::string themeXBT(URIUtils::ReplaceExtension(theme, ".xbt"));
      m_path = URIUtils::AddFileToFolder(mediaDir, "media", themeXBT);
    }
    else
      return false;
  }
  else
    m_path = URIUtils::AddFileToFolder(mediaDir, "media", "Textures.xbt");

  m_path = CSpecialProtocol::TranslatePathConvertCase(m_path);

  // Load the texture file
  if (!m_XBTFReader)
    m_XBTFReader.reset(new CXBTFReader());

  if (!m_XBTFReader->Open(CURL(m_path)))
    return false;

  if (lzo_init() != LZO_E_OK)
    return false;

  return true;
}

void CTextureBundleXBT::Close() const
{
  if (m_XBTFReader)
    m_XBTFReader->Close();
}

bool CTextureBundleXBT::HasFile(const std::string& filename) const
{
  if (!m_XBTFReader || !m_XBTFReader->HasFiles())
    return false;

  return m_XBTFReader->Exists(Normalize(filename));
}

size_t CTextureBundleXBT::GetFileCount() const
{
  if (!m_XBTFReader)
    return 0;

  return m_XBTFReader->GetFileCount();
}

std::vector<CXBTFFile> CTextureBundleXBT::GetFiles() const
{
  if (!m_XBTFReader)
    return std::vector<CXBTFFile>();

  return m_XBTFReader->GetFiles();
}

void CTextureBundleXBT::GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures) const
{
  if (path.size() > 1 && path[1] == ':')
    return;

  if (!m_XBTFReader || !m_XBTFReader->HasFiles())
    return;

  auto testPath = Normalize(path);
  URIUtils::AddSlashAtEnd(testPath);

  auto files = m_XBTFReader->GetFiles();
  for (const auto& file : files)
  {
    auto tmpPath = file.GetPath();
    if (StringUtils::StartsWithNoCase(tmpPath, testPath))
      textures.push_back(tmpPath);
  }
}

bool CTextureBundleXBT::LoadTexture(const std::string& filename, CBaseTexture** ppTexture,
                                     int &width, int &height) const
{
  CXBTFFile file;
  if (!m_XBTFReader)
    return false;

  if (!m_XBTFReader->Get(Normalize(filename), file))
    return false;

  if (file.GetFrames().empty())
    return false;

  auto& frame = file.GetFrames().at(0);
  if (!ConvertFrameToTexture(filename, frame, ppTexture))
  {
    return false;
  }

  width = frame.GetWidth();
  height = frame.GetHeight();

  return true;
}

int CTextureBundleXBT::LoadAnim(const std::string& filename, CBaseTexture*** ppTextures,
                              int &width, int &height, int& nLoops, int** ppDelays) const
{
  CXBTFFile file;
  if (!m_XBTFReader)
    return false;

  if (!m_XBTFReader->Get(Normalize(filename), file))
    return false;

  if (file.GetFrames().empty())
    return false;

  auto nTextures = file.GetFrames().size();
  *ppTextures = new CBaseTexture*[nTextures];
  *ppDelays = new int[nTextures];

  for (size_t i = 0; i < nTextures; i++)
  {
    CXBTFFrame& frame = file.GetFrames().at(i);

    if (!ConvertFrameToTexture(filename, frame, &((*ppTextures)[i])))
    {
      return false;
    }

    (*ppDelays)[i] = frame.GetDuration();
  }

  width = file.GetFrames().at(0).GetWidth();
  height = file.GetFrames().at(0).GetHeight();
  nLoops = file.GetLoop();

  return nTextures;
}

bool CTextureBundleXBT::ConvertFrameToTexture(const std::string& name, CXBTFFrame& frame, CBaseTexture** ppTexture) const
{
  // found texture - allocate the necessary buffers
  std::unique_ptr<unsigned char[]> buffer(new unsigned char[static_cast<size_t>(frame.GetPackedSize())]);
  if (!buffer)
  {
    CLog::Log(LOGERROR, "Out of memory loading texture: %s (need %" PRIu64" bytes)", name.c_str(), frame.GetPackedSize());
    return false;
  }

  // load the compressed texture
  if (!m_XBTFReader->Load(frame, buffer.get()))
  {
    CLog::Log(LOGERROR, "Error loading texture: %s", name.c_str());
    return false;
  }

  // check if it's packed with lzo
  if (frame.IsPacked())
  { // unpack
    std::unique_ptr<unsigned char[]> unpacked(new unsigned char[static_cast<size_t>(frame.GetUnpackedSize())]);
    if (!unpacked)
    {
      CLog::Log(LOGERROR, "Out of memory unpacking texture: %s (need %" PRIu64" bytes)", name.c_str(), frame.GetUnpackedSize());
      return false;
    }
    auto s = static_cast<lzo_uint>(frame.GetUnpackedSize());
    if (lzo1x_decompress_safe(buffer.get(), static_cast<lzo_uint>(frame.GetPackedSize()), unpacked.get(), &s, nullptr) != LZO_E_OK ||
        s != frame.GetUnpackedSize())
    {
      CLog::Log(LOGERROR, "Error loading texture: %s: Decompression error", name.c_str());
      return false;
    }
    buffer.swap(unpacked);
  }

  // create an xbmc texture
  *ppTexture = new CTexture();
  (*ppTexture)->LoadFromMemory(frame.GetWidth(), frame.GetHeight(), 0, frame.GetFormat(), frame.HasAlpha(), buffer.get());

  return true;
}

void CTextureBundleXBT::SetThemeBundle(bool themeBundle)
{
  m_themeBundle = themeBundle;
}

// normalize to how it's stored within the bundle
// lower case + using forward slash rather than back slash
std::string CTextureBundleXBT::Normalize(const std::string &name)
{
  std::string newName(name);
  
  StringUtils::Trim(newName);
  StringUtils::ToLower(newName);
  StringUtils::Replace(newName, '\\','/');

  return newName;
}

uint8_t* CTextureBundleXBT::UnpackFrame(const CXBTFReader& reader, const CXBTFFrame& frame)
{
  std::unique_ptr<uint8_t[]> packedBuffer(new uint8_t[static_cast<size_t>(frame.GetPackedSize())]);
  if (!packedBuffer)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: out of memory loading frame with %" PRIu64" packed bytes", frame.GetPackedSize());
    return nullptr;
  }

  // load the compressed texture
  if (!reader.Load(frame, packedBuffer.get()))
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: error loading frame");
    return nullptr;
  }

  // if the frame isn't packed there's nothing else to be done
  if (!frame.IsPacked())
    return packedBuffer.release();

  std::unique_ptr<uint8_t[]> unpackedBuffer(new uint8_t[static_cast<size_t>(frame.GetUnpackedSize())]);
  if (!unpackedBuffer)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: out of memory loading frame with %" PRIu64" unpacked bytes", frame.GetPackedSize());
    return nullptr;
  }

  // make sure lzo is initialized
  if (lzo_init() != LZO_E_OK)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: failed to initialize lzo");
    return nullptr;
  }

  auto size = static_cast<lzo_uint>(frame.GetUnpackedSize());
  if (lzo1x_decompress_safe(packedBuffer.get(), static_cast<lzo_uint>(frame.GetPackedSize()),
    unpackedBuffer.get(), &size, nullptr)
    != LZO_E_OK || size != frame.GetUnpackedSize())
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: failed to decompress frame with %" PRIu64" unpacked bytes to %" PRIu64" bytes", frame.GetPackedSize(), frame.GetUnpackedSize());
    return nullptr;
  }


  return unpackedBuffer.release();
}

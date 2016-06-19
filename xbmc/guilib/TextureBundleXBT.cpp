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
#include "filesystem/XbtManager.h"
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
  if (m_XBTFReader != nullptr && m_XBTFReader->IsOpen())
  {
    XFILE::CXbtManager::GetInstance().Release(CURL(m_path));
    CLog::Log(LOGDEBUG, "%s - Closed %sbundle", __FUNCTION__, m_themeBundle ? "theme " : "");
  }
}

bool CTextureBundleXBT::OpenBundle()
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
      m_path = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media", themeXBT);
    }
    else
    {
      return false;
    }
  }
  else
  {
    m_path = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media", "Textures.xbt");
  }

  m_path = CSpecialProtocol::TranslatePathConvertCase(m_path);

  // Load the texture file
  if (!XFILE::CXbtManager::GetInstance().GetReader(CURL(m_path), m_XBTFReader))
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - Opened bundle %s", __FUNCTION__, m_path.c_str());

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

void CTextureBundleXBT::GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures)
{
  if (path.size() > 1 && path[1] == ':')
    return;

  if ((m_XBTFReader == nullptr || !m_XBTFReader->IsOpen()) && !OpenBundle())
    return;

  std::string testPath = Normalize(path);
  URIUtils::AddSlashAtEnd(testPath);

  std::vector<CXBTFFile> files = m_XBTFReader->GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    std::string path = files[i].GetPath();
    if (StringUtils::StartsWithNoCase(path, testPath))
      textures.push_back(path);
  }
}

bool CTextureBundleXBT::LoadTexture(const std::string& Filename, CBaseTexture** ppTexture,
                                     int &width, int &height)
{
  std::string name = Normalize(Filename);

  CXBTFFile file;
  if (!m_XBTFReader->Get(name, file))
    return false;

  if (file.GetFrames().empty())
    return false;

  CXBTFFrame& frame = file.GetFrames().at(0);
  if (!ConvertFrameToTexture(Filename, frame, ppTexture))
  {
    return false;
  }

  width = frame.GetWidth();
  height = frame.GetHeight();

  return true;
}

int CTextureBundleXBT::LoadAnim(const std::string& Filename, CBaseTexture*** ppTextures,
                              int &width, int &height, int& nLoops, int** ppDelays)
{
  std::string name = Normalize(Filename);

  CXBTFFile file;
  if (!m_XBTFReader->Get(name, file))
    return false;

  if (file.GetFrames().empty())
    return false;

  size_t nTextures = file.GetFrames().size();
  *ppTextures = new CBaseTexture*[nTextures];
  *ppDelays = new int[nTextures];

  for (size_t i = 0; i < nTextures; i++)
  {
    CXBTFFrame& frame = file.GetFrames().at(i);

    if (!ConvertFrameToTexture(Filename, frame, &((*ppTextures)[i])))
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

bool CTextureBundleXBT::ConvertFrameToTexture(const std::string& name, CXBTFFrame& frame, CBaseTexture** ppTexture)
{
  // found texture - allocate the necessary buffers
  unsigned char *buffer = new unsigned char [(size_t)frame.GetPackedSize()];
  if (buffer == NULL)
  {
    CLog::Log(LOGERROR, "Out of memory loading texture: %s (need %" PRIu64" bytes)", name.c_str(), frame.GetPackedSize());
    return false;
  }

  // load the compressed texture
  if (!m_XBTFReader->Load(frame, buffer))
  {
    CLog::Log(LOGERROR, "Error loading texture: %s", name.c_str());
    delete[] buffer;
    return false;
  }

  // check if it's packed with lzo
  if (frame.IsPacked())
  { // unpack
    unsigned char *unpacked = new unsigned char[(size_t)frame.GetUnpackedSize()];
    if (unpacked == NULL)
    {
      CLog::Log(LOGERROR, "Out of memory unpacking texture: %s (need %" PRIu64" bytes)", name.c_str(), frame.GetUnpackedSize());
      delete[] buffer;
      return false;
    }
    lzo_uint s = (lzo_uint)frame.GetUnpackedSize();
    if (lzo1x_decompress_safe(buffer, (lzo_uint)frame.GetPackedSize(), unpacked, &s, NULL) != LZO_E_OK ||
        s != frame.GetUnpackedSize())
    {
      CLog::Log(LOGERROR, "Error loading texture: %s: Decompression error", name.c_str());
      delete[] buffer;
      delete[] unpacked;
      return false;
    }
    delete[] buffer;
    buffer = unpacked;
  }

  // create an xbmc texture
  *ppTexture = new CTexture();
  (*ppTexture)->LoadFromMemory(frame.GetWidth(), frame.GetHeight(), 0, frame.GetFormat(), frame.HasAlpha(), buffer);

  delete[] buffer;

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
  uint8_t* packedBuffer = new uint8_t[static_cast<size_t>(frame.GetPackedSize())];
  if (packedBuffer == nullptr)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: out of memory loading frame with %" PRIu64" packed bytes", frame.GetPackedSize());
    return nullptr;
  }

  // load the compressed texture
  if (!reader.Load(frame, packedBuffer))
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: error loading frame");
    delete[] packedBuffer;
    return nullptr;
  }

  // if the frame isn't packed there's nothing else to be done
  if (!frame.IsPacked())
    return packedBuffer;

  uint8_t* unpackedBuffer = new uint8_t[static_cast<size_t>(frame.GetUnpackedSize())];
  if (unpackedBuffer == nullptr)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: out of memory loading frame with %" PRIu64" unpacked bytes", frame.GetPackedSize());
    delete[] packedBuffer;
    return nullptr;
  }

  // make sure lzo is initialized
  if (lzo_init() != LZO_E_OK)
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: failed to initialize lzo");
    delete[] packedBuffer;
    delete[] unpackedBuffer;
    return nullptr;
  }

  lzo_uint size = static_cast<lzo_uint>(frame.GetUnpackedSize());
  if (lzo1x_decompress_safe(packedBuffer, static_cast<lzo_uint>(frame.GetPackedSize()), unpackedBuffer, &size, nullptr) != LZO_E_OK || size != frame.GetUnpackedSize())
  {
    CLog::Log(LOGERROR, "CTextureBundleXBT: failed to decompress frame with %" PRIu64" unpacked bytes to %" PRIu64" bytes", frame.GetPackedSize(), frame.GetUnpackedSize());
    delete[] packedBuffer;
    delete[] unpackedBuffer;
    return nullptr;
  }

  delete[] packedBuffer;

  return unpackedBuffer;
}

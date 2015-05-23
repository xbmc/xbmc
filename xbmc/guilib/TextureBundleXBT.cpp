/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "squish.h"
#include "system.h"
#include "TextureBundleXBT.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "utils/log.h"
#include "addons/Skin.h"
#include "settings/Settings.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "XBTF.h"
#include <lzo/lzo1x.h>

#ifdef TARGET_WINDOWS
#pragma comment(lib,"liblzo2.lib")
#endif

CTextureBundleXBT::CTextureBundleXBT(void)
{
  m_themeBundle = false;
  m_TimeStamp = 0;
}

CTextureBundleXBT::~CTextureBundleXBT(void)
{
  Cleanup();
}

bool CTextureBundleXBT::OpenBundle()
{
  Cleanup();

  // Find the correct texture file (skin or theme)
  std::string strPath;

  if (m_themeBundle)
  {
    // if we are the theme bundle, we only load if the user has chosen
    // a valid theme (or the skin has a default one)
    std::string theme = CSettings::Get().GetString("lookandfeel.skintheme");
    if (!theme.empty() && !StringUtils::EqualsNoCase(theme, "SKINDEFAULT"))
    {
      std::string themeXBT(URIUtils::ReplaceExtension(theme, ".xbt"));
      strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media");
      strPath = URIUtils::AddFileToFolder(strPath, themeXBT);
    }
    else
    {
      return false;
    }
  }
  else
  {
    strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media/Textures.xbt");
  }

  strPath = CSpecialProtocol::TranslatePathConvertCase(strPath);

  // Load the texture file
  if (!m_XBTFReader.Open(strPath))
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - Opened bundle %s", __FUNCTION__, strPath.c_str());

  m_TimeStamp = m_XBTFReader.GetLastModificationTimestamp();

  if (lzo_init() != LZO_E_OK)
  {
    return false;
  }

  return true;
}

bool CTextureBundleXBT::HasFile(const std::string& Filename)
{
  if (!m_XBTFReader.IsOpen() && !OpenBundle())
    return false;

  if (m_XBTFReader.GetLastModificationTimestamp() > m_TimeStamp)
  {
    CLog::Log(LOGINFO, "Texture bundle has changed, reloading");
    if (!OpenBundle())
      return false;
  }

  std::string name = Normalize(Filename);
  return m_XBTFReader.Exists(name);
}

void CTextureBundleXBT::GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures)
{
  if (path.size() > 1 && path[1] == ':')
    return;

  if (!m_XBTFReader.IsOpen() && !OpenBundle())
    return;

  std::string testPath = Normalize(path);
  URIUtils::AddSlashAtEnd(testPath);

  std::vector<CXBTFFile>& files = m_XBTFReader.GetFiles();
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

  CXBTFFile* file = m_XBTFReader.Find(name);
  if (!file)
    return false;

  if (file->GetFrames().size() == 0)
    return false;

  CXBTFFrame& frame = file->GetFrames().at(0);
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

  CXBTFFile* file = m_XBTFReader.Find(name);
  if (!file)
    return false;

  if (file->GetFrames().size() == 0)
    return false;

  size_t nTextures = file->GetFrames().size();
  *ppTextures = new CBaseTexture*[nTextures];
  *ppDelays = new int[nTextures];

  for (size_t i = 0; i < nTextures; i++)
  {
    CXBTFFrame& frame = file->GetFrames().at(i);

    if (!ConvertFrameToTexture(Filename, frame, &((*ppTextures)[i])))
    {
      return false;
    }

    (*ppDelays)[i] = frame.GetDuration();
  }

  width = file->GetFrames().at(0).GetWidth();
  height = file->GetFrames().at(0).GetHeight();
  nLoops = file->GetLoop();

  return nTextures;
}

bool CTextureBundleXBT::ConvertFrameToTexture(const std::string& name, CXBTFFrame& frame, CBaseTexture** ppTexture)
{
  // found texture - allocate the necessary buffers
  squish::u8 *buffer = new squish::u8[(size_t)frame.GetPackedSize()];
  if (buffer == NULL)
  {
    CLog::Log(LOGERROR, "Out of memory loading texture: %s (need %" PRIu64" bytes)", name.c_str(), frame.GetPackedSize());
    return false;
  }

  // load the compressed texture
  if (!m_XBTFReader.Load(frame, buffer))
  {
    CLog::Log(LOGERROR, "Error loading texture: %s", name.c_str());
    delete[] buffer;
    return false;
  }

  // check if it's packed with lzo
  if (frame.IsPacked())
  { // unpack
    squish::u8 *unpacked = new squish::u8[(size_t)frame.GetUnpackedSize()];
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

void CTextureBundleXBT::Cleanup()
{
  if (m_XBTFReader.IsOpen())
  {
    m_XBTFReader.Close();
    CLog::Log(LOGDEBUG, "%s - Closed %sbundle", __FUNCTION__, m_themeBundle ? "theme " : "");
  }
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

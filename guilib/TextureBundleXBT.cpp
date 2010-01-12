/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "lib/libsquish/squish.h"
#include "system.h"
#include "TextureBundleXBT.h"
#include "Texture.h"
#include "GraphicContext.h"
#include "utils/log.h"
#include "SkinInfo.h"
#include "GUISettings.h"
#include "Util.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/EndianSwap.h"
#include "XBTF.h"
#include "WindowingFactory.h"
#ifndef _LINUX
#include "lib/liblzo/LZO1X.H"
#else
#include <lzo/lzo1x.h>
#endif

#if !defined(__GNUC__)
#pragma comment(lib,"../../xbmc/lib/liblzo/lzo.lib")
#pragma comment (lib,"../../xbmc/lib/libsquish/lib/squish.lib")
#endif

CTextureBundleXBT::CTextureBundleXBT(void)
{
  m_themeBundle = false;
}

CTextureBundleXBT::~CTextureBundleXBT(void)
{
  Cleanup();
}

bool CTextureBundleXBT::OpenBundle()
{
  Cleanup();

  // Find the correct texture file (skin or theme)
  CStdString strPath;

  if (m_themeBundle)
  {
    // if we are the theme bundle, we only load if the user has chosen
    // a valid theme (or the skin has a default one)
    CStdString themeXBT = g_guiSettings.GetString("lookandfeel.skintheme");
    if (!themeXBT.IsEmpty() && themeXBT.Find(".xbt") != -1 && themeXBT.CompareNoCase("SKINDEFAULT"))
    {
      strPath = CUtil::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media");
      strPath = CUtil::AddFileToFolder(strPath, themeXBT);
    }
    else
    {
      return false;
    }
  }
  else
  {
    strPath = CUtil::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media/Textures.xbt");
  }

  strPath = PTH_IC(strPath);

  // Load the texture file
  if (!m_XBTFReader.Open(strPath))
  {
    return false;
  }

  m_TimeStamp = m_XBTFReader.GetLastModificationTimestamp();

  if (lzo_init() != LZO_E_OK)
  {
    return false;
  }

  return true;
}

bool CTextureBundleXBT::HasFile(const CStdString& Filename)
{
  if (!m_XBTFReader.IsOpen() && !OpenBundle())
    return false;

  if (m_XBTFReader.GetLastModificationTimestamp() > m_TimeStamp)
  {
    CLog::Log(LOGINFO, "Texture bundle has changed, reloading");
    if (!OpenBundle())
      return false;
  }

  CStdString name = Normalize(Filename);
  return m_XBTFReader.Exists(name);
}

void CTextureBundleXBT::GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures)
{
  if (path.GetLength() > 1 && path[1] == ':')
    return;

  if (!m_XBTFReader.IsOpen() && !OpenBundle())
    return;

  CStdString testPath = Normalize(path);
  CUtil::AddSlashAtEnd(testPath);
  int testLength = testPath.GetLength();

  std::vector<CXBTFFile>& files = m_XBTFReader.GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    CStdString path = files[i].GetPath();
    if (path.Left(testLength).Equals(testPath))
      textures.push_back(path);
  }
}

bool CTextureBundleXBT::LoadTexture(const CStdString& Filename, CBaseTexture** ppTexture,
                                     int &width, int &height)
{
  CStdString name = Normalize(Filename);

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

int CTextureBundleXBT::LoadAnim(const CStdString& Filename, CBaseTexture*** ppTextures,
                              int &width, int &height, int& nLoops, int** ppDelays)
{
  CStdString name = Normalize(Filename);

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

bool CTextureBundleXBT::ConvertFrameToTexture(const CStdString& name, CXBTFFrame& frame, CBaseTexture** ppTexture)
{
  // found texture - allocate the necessary buffers
  squish::u8 *buffer = new squish::u8[(size_t)frame.GetPackedSize()];
  if (buffer == NULL)
  {
    CLog::Log(LOGERROR, "Out of memory loading texture: %s (need %llu bytes)", name.c_str(), frame.GetPackedSize());
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
      CLog::Log(LOGERROR, "Out of memory unpacking texture: %s (need %llu bytes)", name.c_str(), frame.GetUnpackedSize());
      delete[] buffer;
      return false;
    }
    lzo_uint s = (lzo_uint)frame.GetUnpackedSize();
    if (lzo1x_decompress(buffer, (lzo_uint)frame.GetPackedSize(), unpacked, &s, NULL) != LZO_E_OK ||
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
  unsigned int format = frame.GetFormat();
  if ((format & XB_FMT_DXT_MASK) != 0 && !g_Windowing.SupportsDXT())
  { // we don't support this texture format, so decode
    squish::u8 *decoded = new squish::u8[frame.GetWidth() * frame.GetHeight() * 4];
    if (decoded == NULL)
    {
      CLog::Log(LOGERROR, "Out of memory unpacking texture: %s (need %llu bytes)", name.c_str(), frame.GetUnpackedSize());
      delete[] buffer;
      return false;
    }

    if (format == XB_FMT_DXT1)
      squish::DecompressImage(decoded, frame.GetWidth(), frame.GetHeight(), buffer, squish::kDxt1 | squish::kSourceBGRA);
    else if (format == XB_FMT_DXT3)
      squish::DecompressImage(decoded, frame.GetWidth(), frame.GetHeight(), buffer, squish::kDxt3 | squish::kSourceBGRA);
    else if (format == XB_FMT_DXT5)
      squish::DecompressImage(decoded, frame.GetWidth(), frame.GetHeight(), buffer, squish::kDxt5 | squish::kSourceBGRA);

    format = XB_FMT_A8R8G8B8;
    delete[] buffer;
    buffer = decoded;
  }

  // create an xbmc texture
  *ppTexture = new CTexture();
  (*ppTexture)->LoadFromMemory(frame.GetWidth(), frame.GetHeight(), 0, format, buffer);

  delete[] buffer;

  return true;
}

void CTextureBundleXBT::Cleanup()
{
  if (m_XBTFReader.IsOpen())
  {
    m_XBTFReader.Close();
  }
}

void CTextureBundleXBT::SetThemeBundle(bool themeBundle)
{
  m_themeBundle = themeBundle;
}

// normalize to how it's stored within the bundle
// lower case + using forward slash rather than back slash
CStdString CTextureBundleXBT::Normalize(const CStdString &name)
{
  CStdString newName(name);
  newName.Normalize();
  newName.Replace('\\','/');

  return newName;
}

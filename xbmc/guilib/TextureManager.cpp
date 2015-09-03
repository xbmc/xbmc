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

#include "TextureManager.h"
#include "Texture.h"
#include "AnimatedGif.h"
#include "GraphicContext.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "addons/Skin.h"
#ifdef _DEBUG_TEXTURES
#include "utils/TimeUtils.h"
#endif
#include "threads/SystemClock.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "URL.h"
#include <assert.h>

#if defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
#include "windowing/WindowingFactory.h" // for g_Windowing in CGUITextureManager::FreeUnusedTextures
#endif

using namespace std;


/************************************************************************/
/*                                                                      */
/************************************************************************/
CTextureArray::CTextureArray(int width, int height, int loops,  bool texCoordsArePixels)
{
  m_width = width;
  m_height = height;
  m_loops = loops;
  m_orientation = 0;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
}

CTextureArray::CTextureArray()
{
  Reset();
}

CTextureArray::~CTextureArray()
{

}

unsigned int CTextureArray::size() const
{
  return m_textures.size();
}


void CTextureArray::Reset()
{
  m_textures.clear();
  m_delays.clear();
  m_width = 0;
  m_height = 0;
  m_loops = 0;
  m_orientation = 0;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
}

void CTextureArray::Add(CBaseTexture *texture, int delay)
{
  if (!texture)
    return;

  m_textures.push_back(texture);
  m_delays.push_back(delay ? delay * 2 : 100);

  m_texWidth = texture->GetTextureWidth();
  m_texHeight = texture->GetTextureHeight();
  m_texCoordsArePixels = false;
}

void CTextureArray::Set(CBaseTexture *texture, int width, int height)
{
  assert(!m_textures.size()); // don't try and set a texture if we already have one!
  m_width = width;
  m_height = height;
  m_orientation = texture ? texture->GetOrientation() : 0;
  Add(texture, 100);
}

void CTextureArray::Free()
{
  CSingleLock lock(g_graphicsContext);
  for (unsigned int i = 0; i < m_textures.size(); i++)
  {
    delete m_textures[i];
  }

  m_textures.clear();
  m_delays.clear();

  Reset();
}


/************************************************************************/
/*                                                                      */
/************************************************************************/

CTextureMap::CTextureMap()
{
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::CTextureMap(const std::string& textureName, int width, int height, int loops)
: m_texture(width, height, loops)
, m_textureName(textureName)
{
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::~CTextureMap()
{
  FreeTexture();
}

bool CTextureMap::Release()
{
  if (!m_texture.m_textures.size())
    return true;
  if (!m_referenceCount)
    return true;

  m_referenceCount--;
  if (!m_referenceCount)
  {
    return true;
  }
  return false;
}

const std::string& CTextureMap::GetName() const
{
  return m_textureName;
}

const CTextureArray& CTextureMap::GetTexture()
{
  m_referenceCount++;
  return m_texture;
}

void CTextureMap::Dump() const
{
  if (!m_referenceCount)
    return;   // nothing to see here

  CLog::Log(LOGDEBUG, "%s: texture:%s has %" PRIuS" frames %i refcount", __FUNCTION__, m_textureName.c_str(), m_texture.m_textures.size(), m_referenceCount);
}

unsigned int CTextureMap::GetMemoryUsage() const
{
  return m_memUsage;
}

void CTextureMap::Flush()
{
  if (!m_referenceCount)
    FreeTexture();
}


void CTextureMap::FreeTexture()
{
  m_texture.Free();
}

bool CTextureMap::IsEmpty() const
{
  return m_texture.m_textures.size() == 0;
}

void CTextureMap::Add(CBaseTexture* texture, int delay)
{
  m_texture.Add(texture, delay);

  if (texture)
    m_memUsage += sizeof(CTexture) + (texture->GetTextureWidth() * texture->GetTextureHeight() * 4);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
CGUITextureManager::CGUITextureManager(void)
{
  // we set the theme bundle to be the first bundle (thus prioritizing it)
  m_TexBundle[0].SetThemeBundle(true);
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
bool CGUITextureManager::CanLoad(const std::string &texturePath)
{
  if (texturePath == "-")
    return false;

  if (!CURL::IsFullPath(texturePath))
    return true;  // assume we have it

  // we can't (or shouldn't) be loading from remote paths, so check these
  return URIUtils::IsHD(texturePath);
}

bool CGUITextureManager::HasTexture(const std::string &textureName, std::string *path, int *bundle, int *size)
{
  // default values
  if (bundle) *bundle = -1;
  if (size) *size = 0;
  if (path) *path = textureName;

  if (!CanLoad(textureName))
    return false;

  // Check our loaded and bundled textures - we store in bundles using \\.
  std::string bundledName = CTextureBundle::Normalize(textureName);
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == textureName)
    {
      if (size) *size = 1;
      return true;
    }
  }

  for (int i = 0; i < 2; i++)
  {
    if (m_TexBundle[i].HasFile(bundledName))
    {
      if (bundle) *bundle = i;
      return true;
    }
  }

  std::string fullPath = GetTexturePath(textureName);
  if (path)
    *path = fullPath;

  return !fullPath.empty();
}

const CTextureArray& CGUITextureManager::Load(const std::string& strTextureName, bool checkBundleOnly /*= false */)
{
  std::string strPath;
  static CTextureArray emptyTexture;
  int bundle = -1;
  int size = 0;
  if (!HasTexture(strTextureName, &strPath, &bundle, &size))
    return emptyTexture;

  if (size) // we found the texture
  {
    for (int i = 0; i < (int)m_vecTextures.size(); ++i)
    {
      CTextureMap *pMap = m_vecTextures[i];
      if (pMap->GetName() == strTextureName)
      {
        //CLog::Log(LOGDEBUG, "Total memusage %u", GetMemoryUsage());
        return pMap->GetTexture();
      }
    }
    // Whoops, not there.
    return emptyTexture;
  }

  for (ilistUnused i = m_unusedTextures.begin(); i != m_unusedTextures.end(); ++i)
  {
    CTextureMap* pMap = i->first;
    if (pMap->GetName() == strTextureName && i->second > 0)
    {
      m_vecTextures.push_back(pMap);
      m_unusedTextures.erase(i);
      return pMap->GetTexture();
    }
  }

  if (checkBundleOnly && bundle == -1)
    return emptyTexture;

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(g_graphicsContext);

#ifdef _DEBUG_TEXTURES
  int64_t start;
  start = CurrentHostCounter();
#endif

  if (StringUtils::EndsWithNoCase(strPath, ".gif"))
  {
    CTextureMap* pMap;

    if (bundle >= 0)
    {
      CBaseTexture **pTextures;
      int nLoops = 0, width = 0, height = 0;
      int* Delay;
      int nImages = m_TexBundle[bundle].LoadAnim(strTextureName, &pTextures, width, height, nLoops, &Delay);
      if (!nImages)
      {
        CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
        delete [] pTextures;
        delete [] Delay;
        return emptyTexture;
      }

      pMap = new CTextureMap(strTextureName, width, height, nLoops);
      for (int iImage = 0; iImage < nImages; ++iImage)
      {
        pMap->Add(pTextures[iImage], Delay[iImage]);
      }

      delete [] pTextures;
      delete [] Delay;
    }
    else
    {
      CAnimatedGifSet AnimatedGifSet;
      int iImages = AnimatedGifSet.LoadGIF(strPath.c_str());
      if (iImages == 0)
      {
        if (StringUtils::StartsWith(strPath, g_SkinInfo->Path()))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return emptyTexture;
      }
      int iWidth = AnimatedGifSet.FrameWidth;
      int iHeight = AnimatedGifSet.FrameHeight;

      // fixup our palette
      COLOR *palette = AnimatedGifSet.m_vecimg[0]->Palette;
      // set the alpha values to fully opaque
      for (int i = 0; i < 256; i++)
        palette[i].x = 0xff;
      // and set the transparent colour
      if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
        palette[AnimatedGifSet.m_vecimg[0]->Transparent].x = 0;

      pMap = new CTextureMap(strTextureName, iWidth, iHeight, AnimatedGifSet.nLoops);

      for (int iImage = 0; iImage < iImages; iImage++)
      {
        CTexture *glTexture = new CTexture();
        if (glTexture)
        {
          CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];
          glTexture->LoadPaletted(pImage->Width, pImage->Height, pImage->BytesPerRow, XB_FMT_A8R8G8B8, (unsigned char *)pImage->Raster, palette);
          pMap->Add(glTexture, pImage->Delay);
        }
      } // of for (int iImage=0; iImage < iImages; iImage++)
    }

    m_vecTextures.push_back(pMap);
    return pMap->GetTexture();
  } // of if (strPath.Right(4).ToLower()==".gif")

  CBaseTexture *pTexture = NULL;
  int width = 0, height = 0;
  if (bundle >= 0)
  {
    if (!m_TexBundle[bundle].LoadTexture(strTextureName, &pTexture, width, height))
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
      return emptyTexture;
    }
  }
  else
  {
    pTexture = CBaseTexture::LoadFromFile(strPath);
    if (!pTexture)
      return emptyTexture;
    width = pTexture->GetWidth();
    height = pTexture->GetHeight();
  }

  if (!pTexture) return emptyTexture;

  CTextureMap* pMap = new CTextureMap(strTextureName, width, height, 0);
  pMap->Add(pTexture, 100);
  m_vecTextures.push_back(pMap);

#ifdef _DEBUG_TEXTURES
  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  char temp[200];
  sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end - start) / freq, (bundle >= 0) ? " (bundled)" : "");
  OutputDebugString(temp);
#endif

  return pMap->GetTexture();
}


void CGUITextureManager::ReleaseTexture(const std::string& strTextureName, bool immediately /*= false */)
{
  CSingleLock lock(g_graphicsContext);

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    if (pMap->GetName() == strTextureName)
    {
      if (pMap->Release())
      {
        //CLog::Log(LOGINFO, "  cleanup:%s", strTextureName.c_str());
        // add to our textures to free
        m_unusedTextures.push_back(make_pair(pMap, immediately ? 0 : XbmcThreads::SystemClockMillis()));
        i = m_vecTextures.erase(i);
      }
      return;
    }
    ++i;
  }
  CLog::Log(LOGWARNING, "%s: Unable to release texture %s", __FUNCTION__, strTextureName.c_str());
}

void CGUITextureManager::FreeUnusedTextures(unsigned int timeDelay)
{
  unsigned int currFrameTime = XbmcThreads::SystemClockMillis();
  CSingleLock lock(g_graphicsContext);
  for (ilistUnused i = m_unusedTextures.begin(); i != m_unusedTextures.end();)
  {
    if (currFrameTime - i->second >= timeDelay)
    {
      delete i->first;
      i = m_unusedTextures.erase(i);
    }
    else
      ++i;
  }

#if defined(HAS_GL) || defined(HAS_GLES)
  for (unsigned int i = 0; i < m_unusedHwTextures.size(); ++i)
  {
  // on ios the hw textures might be deleted from the os
  // when XBMC is backgrounded (e.x. for backgrounded music playback)
  // sanity check before delete in that case.
#if defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
    if (!g_Windowing.IsBackgrounded() || glIsTexture(m_unusedHwTextures[i]))
#endif
      glDeleteTextures(1, (GLuint*) &m_unusedHwTextures[i]);
  }
#endif
  m_unusedHwTextures.clear();
}

void CGUITextureManager::ReleaseHwTexture(unsigned int texture)
{
  CSingleLock lock(g_graphicsContext);
  m_unusedHwTextures.push_back(texture);
}

void CGUITextureManager::Cleanup()
{
  CSingleLock lock(g_graphicsContext);

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    CLog::Log(LOGWARNING, "%s: Having to cleanup texture %s", __FUNCTION__, pMap->GetName().c_str());
    delete pMap;
    i = m_vecTextures.erase(i);
  }
  for (int i = 0; i < 2; i++)
    m_TexBundle[i].Cleanup();
  FreeUnusedTextures();
}

void CGUITextureManager::Dump() const
{
  CLog::Log(LOGDEBUG, "%s: total texturemaps size:%" PRIuS, __FUNCTION__, m_vecTextures.size());

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    const CTextureMap* pMap = m_vecTextures[i];
    if (!pMap->IsEmpty())
      pMap->Dump();
  }
}

void CGUITextureManager::Flush()
{
  CSingleLock lock(g_graphicsContext);

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    pMap->Flush();
    if (pMap->IsEmpty() )
    {
      delete pMap;
      i = m_vecTextures.erase(i);
    }
    else
    {
      ++i;
    }
  }
}

unsigned int CGUITextureManager::GetMemoryUsage() const
{
  unsigned int memUsage = 0;
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    memUsage += m_vecTextures[i]->GetMemoryUsage();
  }
  return memUsage;
}

void CGUITextureManager::SetTexturePath(const std::string &texturePath)
{
  CSingleLock lock(m_section);
  m_texturePaths.clear();
  AddTexturePath(texturePath);
}

void CGUITextureManager::AddTexturePath(const std::string &texturePath)
{
  CSingleLock lock(m_section);
  if (!texturePath.empty())
    m_texturePaths.push_back(texturePath);
}

void CGUITextureManager::RemoveTexturePath(const std::string &texturePath)
{
  CSingleLock lock(m_section);
  for (vector<std::string>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
  {
    if (*it == texturePath)
    {
      m_texturePaths.erase(it);
      return;
    }
  }
}

std::string CGUITextureManager::GetTexturePath(const std::string &textureName, bool directory /* = false */)
{
  if (CURL::IsFullPath(textureName))
    return textureName;
  else
  { // texture doesn't include the full path, so check all fallbacks
    CSingleLock lock(m_section);
    for (vector<std::string>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
    {
      std::string path = URIUtils::AddFileToFolder(it->c_str(), "media");
      path = URIUtils::AddFileToFolder(path, textureName);
      if (directory)
      {
        if (XFILE::CDirectory::Exists(path))
          return path;
      }
      else
      {
        if (XFILE::CFile::Exists(path))
          return path;
      }
    }
  }
  return "";
}

void CGUITextureManager::GetBundledTexturesFromPath(const std::string& texturePath, std::vector<std::string> &items)
{
  m_TexBundle[0].GetTexturesFromPath(texturePath, items);
  if (items.empty())
    m_TexBundle[1].GetTexturesFromPath(texturePath, items);
}

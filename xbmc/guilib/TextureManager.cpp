/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureManager.h"

#include <cassert>

#include "addons/Skin.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "windowing/GraphicContext.h"
#include "Texture.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#ifdef _DEBUG_TEXTURES
#include "utils/TimeUtils.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#define WIN_SYSTEM_CLASS CWinSystemIOS
#include "ServiceBroker.h"
#include "windowing/ios/WinSystemIOS.h" // for g_Windowing in CGUITextureManager::FreeUnusedTextures
#elif defined(TARGET_DARWIN_TVOS)
#define WIN_SYSTEM_CLASS CWinSystemTVOS
#include "ServiceBroker.h"
#include "windowing/tvos/WinSystemTVOS.h" // for g_Windowing in CGUITextureManager::FreeUnusedTextures
#endif
#include "FFmpegImage.h"

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

CTextureArray::~CTextureArray() = default;

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
  m_delays.push_back(delay);

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
  Add(texture, 2);
}

void CTextureArray::Free()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
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

  CLog::Log(LOGDEBUG, "{0}: texture:{1} has {2} frames {3} refcount", __FUNCTION__, m_textureName.c_str(),
    m_texture.m_textures.size(), m_referenceCount);
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

void CTextureMap::SetHeight(int height)
{
  m_texture.m_height = height;
}

void CTextureMap::SetWidth(int height)
{
  m_texture.m_width = height;
}

bool CTextureMap::IsEmpty() const
{
  return m_texture.m_textures.empty();
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
  if (texturePath.empty())
    return false;

  if (!CURL::IsFullPath(texturePath))
    return true;  // assume we have it

  // we can't (or shouldn't) be loading from remote paths, so check these
  return URIUtils::IsHD(texturePath);
}

bool CGUITextureManager::HasTexture(const std::string &textureName, std::string *path, int *bundle, int *size)
{
  CSingleLock lock(m_section);

  // default values
  if (bundle) *bundle = -1;
  if (size) *size = 0;
  if (path) *path = textureName;

  if (textureName.empty())
    return false;

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

  if (strTextureName.empty())
    return emptyTexture;

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
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

#ifdef _DEBUG_TEXTURES
  int64_t start;
  start = CurrentHostCounter();
#endif

  if (bundle >= 0 && StringUtils::EndsWithNoCase(strPath, ".gif"))
  {
    CTextureMap* pMap = nullptr;
    CBaseTexture **pTextures = nullptr;
    int nLoops = 0, width = 0, height = 0;
    int* Delay = nullptr;
    int nImages = m_TexBundle[bundle].LoadAnim(strTextureName, &pTextures, width, height, nLoops, &Delay);
    if (!nImages)
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
      delete[] pTextures;
      delete[] Delay;
      return emptyTexture;
    }

    unsigned int maxWidth = 0;
    unsigned int maxHeight = 0;
    pMap = new CTextureMap(strTextureName, width, height, nLoops);
    for (int iImage = 0; iImage < nImages; ++iImage)
    {
      pMap->Add(pTextures[iImage], Delay[iImage]);
      maxWidth = std::max(maxWidth, pTextures[iImage]->GetWidth());
      maxHeight = std::max(maxHeight, pTextures[iImage]->GetHeight());
    }

    pMap->SetWidth((int)maxWidth);
    pMap->SetHeight((int)maxHeight);

    delete[] pTextures;
    delete[] Delay;

    m_vecTextures.push_back(pMap);
    return pMap->GetTexture();
  }
  else if (StringUtils::EndsWithNoCase(strPath, ".gif") ||
           StringUtils::EndsWithNoCase(strPath, ".apng"))
  {
    std::string mimeType;
    if (StringUtils::EndsWithNoCase(strPath, ".gif"))
      mimeType = "image/gif";
    else if (StringUtils::EndsWithNoCase(strPath, ".apng"))
      mimeType = "image/apng";

    XFILE::CFile file;
    XFILE::auto_buffer buf;
    CFFmpegImage anim(mimeType);

    if (file.LoadFile(strPath, buf) <= 0 ||
       !anim.Initialize((uint8_t*)buf.get(), buf.size()))
    {
      CLog::Log(LOGERROR, "Texture manager unable to load file: %s", CURL::GetRedacted(strPath).c_str());
      file.Close();
      return emptyTexture;
    }

    CTextureMap* pMap = new CTextureMap(strTextureName, 0, 0, 0);
    unsigned int maxWidth = 0;
    unsigned int maxHeight = 0;
    uint64_t maxMemoryUsage = 91238400;// 1920*1080*4*11 bytes, i.e, a total of approx. 12 full hd frames

    auto frame = anim.ReadFrame();
    while (frame)
    {
      CTexture *glTexture = new CTexture();
      if (glTexture)
      {
        glTexture->LoadFromMemory(anim.Width(), anim.Height(), frame->GetPitch(), XB_FMT_A8R8G8B8, true, frame->m_pImage);
        pMap->Add(glTexture, frame->m_delay);
        maxWidth = std::max(maxWidth, glTexture->GetWidth());
        maxHeight = std::max(maxHeight, glTexture->GetHeight());
      }

      if (pMap->GetMemoryUsage() <= maxMemoryUsage)
      {
        frame = anim.ReadFrame();
      }
      else
      {
        CLog::Log(LOGDEBUG, "Memory limit (%" PRIu64 " bytes) exceeded, %i frames extracted from file : %s", (maxMemoryUsage/11)*12,pMap->GetTexture().size(), CURL::GetRedacted(strPath).c_str());
        break;
      }
    }

    pMap->SetWidth((int)maxWidth);
    pMap->SetHeight((int)maxHeight);

    file.Close();

    m_vecTextures.push_back(pMap);
    return pMap->GetTexture();
  }

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
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

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
        m_unusedTextures.emplace_back(pMap, immediately ? 0 : XbmcThreads::SystemClockMillis());
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
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
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
    // on ios/tvos the hw textures might be deleted from the os
    // when XBMC is backgrounded (e.x. for backgrounded music playback)
    // sanity check before delete in that case.
#if defined(TARGET_DARWIN_EMBEDDED)
    auto winSystem = dynamic_cast<WIN_SYSTEM_CLASS*>(CServiceBroker::GetWinSystem());
    if (!winSystem->IsBackgrounded() || glIsTexture(m_unusedHwTextures[i]))
#endif
      glDeleteTextures(1, (GLuint*) &m_unusedHwTextures[i]);
  }
#endif
  m_unusedHwTextures.clear();
}

void CGUITextureManager::ReleaseHwTexture(unsigned int texture)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  m_unusedHwTextures.push_back(texture);
}

void CGUITextureManager::Cleanup()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
    CLog::Log(LOGWARNING, "%s: Having to cleanup texture %s", __FUNCTION__, pMap->GetName().c_str());
    delete pMap;
    i = m_vecTextures.erase(i);
  }
  m_TexBundle[0].Close();
  m_TexBundle[1].Close();
  m_TexBundle[0] = CTextureBundle(true);
  m_TexBundle[1] = CTextureBundle();
  FreeUnusedTextures();
}

void CGUITextureManager::Dump() const
{
  CLog::Log(LOGDEBUG, "{0}: total texturemaps size: {1}", __FUNCTION__, m_vecTextures.size());

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    const CTextureMap* pMap = m_vecTextures[i];
    if (!pMap->IsEmpty())
      pMap->Dump();
  }
}

void CGUITextureManager::Flush()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

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
  for (std::vector<std::string>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
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
    for (const std::string& it : m_texturePaths)
    {
      std::string path = URIUtils::AddFileToFolder(it.c_str(), "media", textureName);
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

  CLog::Log(LOGDEBUG, "[Warning] CGUITextureManager::GetTexturePath: could not find texture '%s'", textureName.c_str());
  return "";
}

void CGUITextureManager::GetBundledTexturesFromPath(const std::string& texturePath, std::vector<std::string> &items)
{
  m_TexBundle[0].GetTexturesFromPath(texturePath, items);
  if (items.empty())
    m_TexBundle[1].GetTexturesFromPath(texturePath, items);
}

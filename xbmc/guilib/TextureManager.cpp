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

#include "TextureManager.h"

#include <utility>
#include <cassert>

#include "addons/Skin.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "GraphicContext.h"
#include "system.h"
#include "Texture.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/ScopeGuard.h"

#ifdef _DEBUG_TEXTURES
#include "utils/TimeUtils.h"
#endif
#if defined(TARGET_DARWIN_IOS)
#include "windowing/WindowingFactory.h" // for g_Windowing in CGUITextureManager::FreeUnusedTextures
#endif
#include "FFmpegImage.h"

CTextureArray::CTextureArray(int width, int height, int loops,  bool texCoordsArePixels)
  : m_width(width)
  , m_height(height)
  , m_orientation(0)
  , m_loops(loops)
  , m_texWidth(0)
  , m_texHeight(0)
  , m_texCoordsArePixels(false)
{
}

CTextureArray::CTextureArray()
{
  Reset();
}

std::size_t CTextureArray::size() const
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
  CSingleLock lock(g_graphicsContext);
  for (auto tex : m_textures)
    delete tex;

  m_textures.clear();
  m_delays.clear();

  Reset();
}


/************************************************************************/
/*                                                                      */
/************************************************************************/

CTextureMap::CTextureMap()
  : m_referenceCount(0)
  , m_memUsage(0)
{
}

CTextureMap::CTextureMap(const std::string& textureName, int width, int height, int loops)
  : m_texture(width, height, loops)
  , m_textureName(textureName)
  , m_referenceCount(0)
  , m_memUsage(0)
{
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

void CTextureMap::SetHeight(int height)
{
  m_texture.m_height = height;
}

void CTextureMap::SetWidth(int width)
{
  m_texture.m_width = width;
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
CGUITextureManager::CGUITextureManager()
{
  // we set the theme bundle to be the first bundle (thus prioritizing it)
  m_TexBundle[0].SetThemeBundle(true);
}

CGUITextureManager::~CGUITextureManager()
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

const CTextureArray* CGUITextureManager::LoadInternal(CTextureBundleXBT& bundle, std::string textureName, std::string path)
{
  if (StringUtils::EndsWithNoCase(path, ".gif"))
    return GetTextureGif(bundle, textureName);

  if (StringUtils::EndsWithNoCase(path, ".gif") ||
    StringUtils::EndsWithNoCase(path, ".apng"))
    return GetTextureGifOrPng(bundle, textureName, path);

  return GetTexture(bundle, textureName, path);
}

void CGUITextureManager::LoadAll(CTextureBundleXBT& bundle)
{
  auto files = bundle.GetFiles();
  for (auto& file : files)
  {
    auto textureName = CTextureBundleXBT::Normalize(file.GetPath());
    auto path = GetTexturePath(textureName);

    auto iter = m_vecTextures.find(textureName);
    if (iter != m_vecTextures.end())
      continue;

    LoadInternal(bundle, textureName, path);
  }
}

bool CGUITextureManager::HasTexture(const std::string &textureName, std::string *path, int *bundle, int *size)
{
  // default values
  if (bundle)
    *bundle = -1;
  if (size)
    *size = 0;
  if (path)
    *path = textureName;

  if (textureName.empty())
    return false;

  if (!CanLoad(textureName))
    return false;

  // Check our loaded and bundled textures - we store in bundles using \\.
  auto bundledName = CTextureBundleXBT::Normalize(textureName);
  auto iter = m_vecTextures.find(textureName);
  if (iter != m_vecTextures.end())
  {
    *size = 1;
    return true;
  }

  for (int i = 0; i < 2; ++i)
  {
    if (m_TexBundle[i].HasFile(bundledName))
    {
      if (bundle)
        *bundle = i;

      return true;
    }
  }

  auto fullPath = GetTexturePath(textureName);
  if (path)
    *path = fullPath;

  return !fullPath.empty();
}

const CTextureArray* CGUITextureManager::GetTextureGif(CTextureBundleXBT& bundle, const std::string& strTextureName)
{
  CBaseTexture **pTextures = nullptr;
  int nLoops = 0, width = 0, height = 0;
  int* Delay = nullptr;
  int nImages = bundle.LoadAnim(strTextureName, &pTextures, width, height, nLoops, &Delay);
  if (!nImages)
  {
    CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
    delete[] pTextures;
    delete[] Delay;
    return nullptr;
  }

  unsigned int maxWidth = 0;
  unsigned int maxHeight = 0;
  auto pMap = new CTextureMap(strTextureName, width, height, nLoops);

  for (auto iImage = 0; iImage < nImages; ++iImage)
  {
    pMap->Add(pTextures[iImage], Delay[iImage]);
    maxWidth = std::max(maxWidth, pTextures[iImage]->GetWidth());
    maxHeight = std::max(maxHeight, pTextures[iImage]->GetHeight());
  }

  pMap->SetWidth(static_cast<int>(maxWidth));
  pMap->SetHeight(static_cast<int>(maxHeight));

  delete[] pTextures;
  delete[] Delay;

  m_vecTextures.insert(std::make_pair(strTextureName, std::unique_ptr<CTextureMap>(pMap)));
  return &pMap->GetTexture();
}

const CTextureArray* CGUITextureManager::GetTextureGifOrPng(CTextureBundleXBT& bundle,
                                                            const std::string& strTextureName, std::string strPath)
{

  std::string mimeType;
  if (StringUtils::EndsWithNoCase(strPath, ".gif"))
    mimeType = "image/gif";
  else if (StringUtils::EndsWithNoCase(strPath, ".apng"))
    mimeType = "image/apng";

  XFILE::CFile file;
  XFILE::auto_buffer buf;
  CFFmpegImage anim(mimeType);

  auto pMap = new CTextureMap(strTextureName, 0, 0, 0);

  if (file.LoadFile(strPath, buf) <= 0 ||
    !anim.Initialize(reinterpret_cast<uint8_t*>(buf.get()), buf.size()))
  {
    CLog::Log(LOGERROR, "Texture manager unable to load file: %s", CURL::GetRedacted(strPath).c_str());
    file.Close();
    return nullptr;
  }

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

  pMap->SetWidth(static_cast<int>(maxWidth));
  pMap->SetHeight(static_cast<int>(maxHeight));

  file.Close();

  m_vecTextures.insert(std::make_pair(strTextureName, std::unique_ptr<CTextureMap>(pMap)));
  return &pMap->GetTexture();
}

const CTextureArray* CGUITextureManager::GetTexture(CTextureBundleXBT& bundle, const std::string& strTextureName, std::string strPath)
{
  CBaseTexture *pTexture = nullptr;
  int width = 0, height = 0;
  if (!bundle.LoadTexture(strTextureName, &pTexture, width, height))
  {
    CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
  }

  pTexture = CBaseTexture::LoadFromFile(strPath);
  if (!pTexture)
    return nullptr;
  width = pTexture->GetWidth();
  height = pTexture->GetHeight();

  if (!pTexture)
    return nullptr;

  auto pMap = new CTextureMap(strTextureName, width, height, 0);
  pMap->Add(pTexture, 100);
  m_vecTextures.insert(std::make_pair(strTextureName, std::unique_ptr<CTextureMap>(pMap)));

#ifdef _DEBUG_TEXTURES
  int64_t end, freq;
  end = CurrentHostCounter();
  freq = CurrentHostFrequency();
  char temp[200];
  sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end - start) / freq, (bundle >= 0) ? " (bundled)" : "");
  OutputDebugString(temp);
#endif

  return &pMap->GetTexture();
}

const CTextureArray& CGUITextureManager::Load(const std::string& strTextureName, bool checkBundleOnly /*= false */)
{
  std::string strPath;
  static CTextureArray emptyTexture;
  int bundle = -1;
  int size = 0;

  // To avoid keeping Textures.xbt open and blocking skin updates we
  // make sure that we never leave this method with an open file handle
  auto deleter = [](CTextureBundleXBT* tex)
  {
    tex->Close();
  };

  using TextureGuard = KODI::UTILS::CScopeGuard<CTextureBundleXBT*, nullptr, void(CTextureBundleXBT*)>;
  TextureGuard tg1(deleter, &m_TexBundle[0]);
  TextureGuard tg2(deleter, &m_TexBundle[1]);

  m_TexBundle[0].Open();
  m_TexBundle[1].Open();

  if (strTextureName.empty())
    return emptyTexture;

  if (!HasTexture(strTextureName, &strPath, &bundle, &size))
    return emptyTexture;

  if (size) // we found the texture
  {
    auto iter = m_vecTextures.find(strTextureName);
    if (iter == m_vecTextures.end())
      return emptyTexture;

    return iter->second->GetTexture();
  }

  if (checkBundleOnly && bundle == -1)
    return emptyTexture;
  
  if (bundle == -1)
  {
    auto iter = m_vecTextures.find(strTextureName);
    if (iter != m_vecTextures.end())
      return iter->second->GetTexture();

    auto texture = LoadInternal(m_TexBundle[0], strTextureName, strPath);
    if (texture)
      return *texture;
  }

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(g_graphicsContext);

#ifdef _DEBUG_TEXTURES
  int64_t start;
  start = CurrentHostCounter();
#endif

  if (!m_initialized)
  {
    LoadAll(m_TexBundle[0]);
    LoadAll(m_TexBundle[1]);
    if (m_vecTextures.size() > 0)
      m_initialized = true;
  }

  auto iter = m_vecTextures.find(strTextureName);
  if (iter != m_vecTextures.end())
    return iter->second->GetTexture();

  return emptyTexture;
}

void CGUITextureManager::ReleaseTexture(const std::string& strTextureName, bool immediately /*= false */)
{
  CLog::Log(LOGDEBUG, "%s: Released texture %s", __FUNCTION__, strTextureName.c_str());
}

void CGUITextureManager::FreeUnusedTextures(unsigned int timeDelay)
{
}

void CGUITextureManager::ReleaseHwTexture(unsigned int texture)
{
  CSingleLock lock(g_graphicsContext);
#if defined(HAS_GL) || defined(HAS_GLES)
  // on ios the hw textures might be deleted from the os
  // when XBMC is backgrounded (e.x. for backgrounded music playback)
  // sanity check before delete in that case.
#if defined(TARGET_DARWIN_IOS)
    if (!g_Windowing.IsBackgrounded() || glIsTexture(texture))
#endif
      glDeleteTextures(1, (GLuint*) &texture);
  }
#endif
}

void CGUITextureManager::Cleanup()
{
  CSingleLock lock(g_graphicsContext);

  m_vecTextures.clear();
  m_initialized = false;

  m_TexBundle[0] = CTextureBundleXBT();
  m_TexBundle[1] = CTextureBundleXBT();

  FreeUnusedTextures();
}

void CGUITextureManager::Dump() const
{
  CLog::Log(LOGDEBUG, "%s: total texturemaps size:%" PRIuS, __FUNCTION__, m_vecTextures.size());

  for (const auto& pMap : m_vecTextures)
  {
    if (!pMap.second->IsEmpty())
      pMap.second->Dump();
  }
}

void CGUITextureManager::Flush()
{
  CSingleLock lock(g_graphicsContext);

  auto i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    (*i).second->Flush();
    if ((*i).second->IsEmpty() )
    {
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
  for (const auto& pMap : m_vecTextures)
    memUsage += pMap.second->GetMemoryUsage();

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
  auto it = std::remove(m_texturePaths.begin(), m_texturePaths.end(), texturePath);
  m_texturePaths.erase(it);
}

std::string CGUITextureManager::GetTexturePath(const std::string &textureName, bool directory /* = false */)
{
  if (CURL::IsFullPath(textureName))
    return textureName;

  // texture doesn't include the full path, so check all fallbacks
  CSingleLock lock(m_section);
  for (auto& it : m_texturePaths)
  {
    auto path = URIUtils::AddFileToFolder(it.c_str(), "media", textureName);
    if (directory && XFILE::CDirectory::Exists(path))
        return path;

    if (!directory && XFILE::CFile::Exists(path))
      return path;
  }

  CLog::Log(LOGERROR, "CGUITextureManager::GetTexturePath: could not find texture '%s'", textureName.c_str());
  return std::string();
}

void CGUITextureManager::GetBundledTexturesFromPath(const std::string& texturePath, std::vector<std::string> &items) const
{
  m_TexBundle[0].GetTexturesFromPath(texturePath, items);
  if (items.empty())
    m_TexBundle[1].GetTexturesFromPath(texturePath, items);
}

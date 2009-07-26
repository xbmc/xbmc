/*
*      Copyright (C) 2005-2008 Team XBMC
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

#include "include.h"
#include "TextureManager.h"
#include "AnimatedGif.h"
#include "GraphicContext.h"
#include "Surface.h"
#include "../xbmc/Picture.h"
#include "utils/SingleLock.h"
#include "StringUtils.h"
#include "utils/CharsetConverter.h"
#include "../xbmc/Util.h"
#include "../xbmc/FileSystem/File.h"
#include "../xbmc/FileSystem/Directory.h"
#include "../xbmc/FileSystem/SpecialProtocol.h"

#ifdef HAS_SDL
#define MAX_PICTURE_WIDTH  4096
#define MAX_PICTURE_HEIGHT 4096
#endif

using namespace std;

extern "C" void dllprintf( const char *format, ... );

CGUITextureManager g_TextureManager;

/************************************************************************/
/*                                                                      */
/************************************************************************/
CBaseTexture::CBaseTexture(XBMC::SurfacePtr surface, bool load, bool freeSurface)
{
  m_loadedToGPU = false;
  id = 0;
  m_pixels = NULL;
}

CBaseTexture::~CBaseTexture()
{

}

/************************************************************************/
/*                                                                      */
/************************************************************************/
CTextureArray::CTextureArray(int width, int height, int loops,  bool texCoordsArePixels)
{
  m_width = width;
  m_height = height;
  m_loops = loops;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
};

CTextureArray::CTextureArray()
{
  Reset();
};

CTextureArray::~CTextureArray()
{

};

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
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
};

void CTextureArray::Add(CBaseTexture *texture, int delay)
{
  if (!texture)
    return;

  m_textures.push_back(texture);
  m_delays.push_back(delay ? delay * 2 : 100);

  m_texWidth = texture->textureWidth;
  m_texHeight = texture->textureHeight;
  m_texCoordsArePixels = false;
}

void CTextureArray::Set(CBaseTexture *texture, int width, int height)
{
  assert(!m_textures.size()); // don't try and set a texture if we already have one!
  m_width = width;
  m_height = height;
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
  m_textureName = "";
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::CTextureMap(const CStdString& textureName, int width, int height, int loops)
: m_texture(width, height, loops)
{
  m_textureName = textureName;
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
    FreeTexture();
    return true;
  }
  return false;
}

const CStdString& CTextureMap::GetName() const
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

  CStdString strLog;
  strLog.Format("  texture:%s has %i frames %i refcount\n", m_textureName.c_str(), m_texture.m_textures.size(), m_referenceCount);
  OutputDebugString(strLog.c_str());
}

DWORD CTextureMap::GetMemoryUsage() const
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
  //CGLTexture *glTexture = new CGLTexture(pSurface, false);
  m_texture.Add(texture, delay);

  if (texture)
    m_memUsage += sizeof(CBaseTexture) + (texture->textureWidth * texture->textureHeight * 4); 
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
CGUITextureManager::CGUITextureManager(void)
{
  // we set the theme bundle to be the first bundle (thus prioritizing it)
#if defined(_WIN32)
  // Hack for SDL library that keeps loading and unloading these
  LoadLibraryEx("zlib1.dll", NULL, 0);
  LoadLibraryEx("libpng12-0.dll", NULL, 0);
  LoadLibraryEx("jpeg.dll", NULL, 0);
#endif
  m_TexBundle[0].SetThemeBundle(true);
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}

const CTextureArray& CGUITextureManager::GetTexture(const CStdString& strTextureName)
{
  static CTextureArray emptyTexture;
  //  CLog::Log(LOGINFO, " refcount++ for  GetTexture(%s)\n", strTextureName.c_str());
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      //CLog::Log(LOGDEBUG, "Total memusage %u", GetMemoryUsage());
      return pMap->GetTexture();
    }
  }
  return emptyTexture;
}




/************************************************************************/
/*                                                                      */
/************************************************************************/
bool CGUITextureManager::CanLoad(const CStdString &texturePath) const
{
  if (texturePath == "-")
    return false;

  if (!CURL::IsFullPath(texturePath))
    return true;  // assume we have it

  // we can't (or shouldn't) be loading from remote paths, so check these
  return CUtil::IsHD(texturePath);
}

bool CGUITextureManager::HasTexture(const CStdString &textureName, CStdString *path, int *bundle, int *size)
{
  // default values
  if (bundle) *bundle = -1;
  if (size) *size = 0;
  if (path) *path = textureName;

  if (!CanLoad(textureName))
    return false;

  // Check our loaded and bundled textures - we store in bundles using \\.
  CStdString bundledName = CTextureBundle::Normalize(textureName);
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

  CStdString fullPath = GetTexturePath(textureName);
  if (path)
    *path = fullPath;

  return !fullPath.IsEmpty();
}

int CGUITextureManager::Load(const CStdString& strTextureName, bool checkBundleOnly /*= false */)
{
  CStdString strPath;
  int bundle = -1;
  int size = 0;
  if (!HasTexture(strTextureName, &strPath, &bundle, &size))
    return 0;

  if (size) // we found the texture
    return size;

  if (checkBundleOnly && bundle == -1)
    return 0;

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(g_graphicsContext);

  XBMC::TexturePtr pTexture;
  XBMC::PalettePtr pPal = NULL;

#ifdef _DEBUG
  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);
#endif

  D3DXIMAGE_INFO info;

  if (strPath.Right(4).ToLower() == ".gif")
  {
    CTextureMap* pMap;

    if (bundle >= 0)
    {
      XBMC::TexturePtr *pTextures;
      int nLoops = 0;
      int* Delay;
      int nImages = m_TexBundle[bundle].LoadAnim(strTextureName, &info, &pTextures, &pPal, nLoops, &Delay);
      if (!nImages)
      {
        CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
        return 0;
      }

      pMap = new CTextureMap(strTextureName, info.Width, info.Height, nLoops);
      for (int iImage = 0; iImage < nImages; ++iImage)
      {
        CTexture *newTexture = new CTexture(pTextures[iImage], false);
        pMap->Add(newTexture, Delay[iImage]);
        DELETE_TEXTURE(pTextures[iImage]);
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
        if (!strnicmp(strPath.c_str(), "special://home/skin/", 20) && !strnicmp(strPath.c_str(), "special://xbmc/skin/", 20))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return 0;
      }
      int iWidth = AnimatedGifSet.FrameWidth;
      int iHeight = AnimatedGifSet.FrameHeight;

      int iPaletteSize = (1 << AnimatedGifSet.m_vecimg[0]->BPP);
      pMap = new CTextureMap(strTextureName, iWidth, iHeight, AnimatedGifSet.nLoops);

      for (int iImage = 0; iImage < iImages; iImage++)
      {
        int w = iWidth;
        int h = iHeight;

        pTexture = CREATE_TEXTURE(SDL_HWSURFACE, w, h, 32, RMASK, GMASK, BMASK, AMASK);
        if (pTexture)

        {
          CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];

          if (LOCK_TEXTURE(pTexture) != -1)          
          {
            COLOR *palette = AnimatedGifSet.m_vecimg[0]->Palette;
            // set the alpha values to fully opaque
            for (int i = 0; i < iPaletteSize; i++)
              palette[i].x = 0xff;
            // and set the transparent colour
            if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
              palette[AnimatedGifSet.m_vecimg[0]->Transparent].x = 0;

            for (int y = 0; y < pImage->Height; y++)
            {

              BYTE *dest = (BYTE *)pTexture->pixels + (y * w * 4);
              BYTE *source = (BYTE *)pImage->Raster + y * pImage->BytesPerRow;
              for (int x = 0; x < pImage->Width; x++)
              {
                COLOR col = palette[*source++];
                *dest++ = col.b;
                *dest++ = col.g;
                *dest++ = col.r;
                *dest++ = col.x;
              }
            }
            UNLOCK_TEXTURE(pTexture);
            CTexture *glTexture = new CTexture(pTexture, false);
            pMap->Add(glTexture, pImage->Delay);
            DELETE_TEXTURE(pTexture);
          }
        }
      } // of for (int iImage=0; iImage < iImages; iImage++)
    }

#ifdef _DEBUG
    LARGE_INTEGER end, freq;
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);
    char temp[200];
    sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, (bundle >= 0) ? " (bundled)" : "");
    OutputDebugString(temp);
#endif

    m_vecTextures.push_back(pMap);
    return 1;
  } // of if (strPath.Right(4).ToLower()==".gif")

  if (bundle >= 0)
  {
    if (FAILED(m_TexBundle[bundle].LoadTexture(strTextureName, &info, &pTexture, &pPal)))  
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
      return 0;
    }
  }
  else
  {
    // normal picture
    // convert from utf8
    CStdString texturePath;
    g_charsetConverter.utf8ToStringCharset(strPath, texturePath);

    CPicture pic;
    XBMC::SurfacePtr original = pic.Load(texturePath, MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT);
    if (!original)
    {
      CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
      return 0;
    }
    // make sure the texture format is correct
    XBMC::PixelFormat format;
    format.palette = 0; format.colorkey = 0; format.alpha = 0;
    format.BitsPerPixel = 32; format.BytesPerPixel = 4;
    format.Amask = AMASK; format.Ashift = PIXEL_ASHIFT;
    format.Rmask = RMASK; format.Rshift = PIXEL_RSHIFT;
    format.Gmask = GMASK; format.Gshift = PIXEL_GSHIFT;
    format.Bmask = BMASK; format.Bshift = PIXEL_BSHIFT;
    pTexture = SDL_ConvertSurface(original, &format, SDL_SWSURFACE);

    DELETE_SURFACE(original);
    if (!pTexture)
    {
      CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
      return 0;
    }
    info.Width = pTexture->w;
    info.Height = pTexture->h;
  }

  CTextureMap* pMap = new CTextureMap(strTextureName, info.Width, info.Height, 0);
  CTexture *glTexture = new CTexture(pTexture, false);
  pMap->Add(glTexture, 100);
  m_vecTextures.push_back(pMap);

  DELETE_TEXTURE(pTexture);

#ifdef _DEBUG_TEXTURES
  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  char temp[200];
  sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, (bundle >= 0) ? " (bundled)" : "");
  OutputDebugString(temp);
#endif

  return 1;
}


void CGUITextureManager::ReleaseTexture(const CStdString& strTextureName)
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
        delete pMap;
        i = m_vecTextures.erase(i);
      }
      return;
    }
    ++i;
  }
  CLog::Log(LOGWARNING, "%s: Unable to release texture %s", __FUNCTION__, strTextureName.c_str());
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
}

void CGUITextureManager::Dump() const
{
  CStdString strLog;
  strLog.Format("total texturemaps size:%i\n", m_vecTextures.size());
  OutputDebugString(strLog.c_str());

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

DWORD CGUITextureManager::GetMemoryUsage() const
{
  DWORD memUsage = 0;
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    memUsage += m_vecTextures[i]->GetMemoryUsage();
  }
  return memUsage;
}

void CGUITextureManager::SetTexturePath(const CStdString &texturePath)
{
  m_texturePaths.clear();
  AddTexturePath(texturePath);
}

void CGUITextureManager::AddTexturePath(const CStdString &texturePath)
{
  if (!texturePath.IsEmpty())
    m_texturePaths.push_back(texturePath);
}

void CGUITextureManager::RemoveTexturePath(const CStdString &texturePath)
{
  for (vector<CStdString>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
  {
    if (*it == texturePath)
    {
      m_texturePaths.erase(it);
      return;
    }
  }
}

CStdString CGUITextureManager::GetTexturePath(const CStdString &textureName, bool directory /* = false */)
{
  if (CURL::IsFullPath(textureName))
    return textureName;
  else
  { // texture doesn't include the full path, so check all fallbacks
    for (vector<CStdString>::iterator it = m_texturePaths.begin(); it != m_texturePaths.end(); ++it)
    {
      CStdString path = CUtil::AddFileToFolder(it->c_str(), "media");
      path = CUtil::AddFileToFolder(path, textureName);
      if (directory)
      {
        if (DIRECTORY::CDirectory::Exists(path))
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

void CGUITextureManager::GetBundledTexturesFromPath(const CStdString& texturePath, std::vector<CStdString> &items)
{
  m_TexBundle[0].GetTexturesFromPath(texturePath, items);
  if (items.empty())
    m_TexBundle[1].GetTexturesFromPath(texturePath, items);
}

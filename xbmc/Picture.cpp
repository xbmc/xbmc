/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Picture.h"
#include "Util.h"
#include "TextureManager.h"

using namespace XFILE;

CPicture::CPicture(void)
{
  ZeroMemory(&m_info, sizeof(ImageInfo));
}

CPicture::~CPicture(void)
{

}

#ifndef HAS_SDL
IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
#else
SDL_Surface* CPicture::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
#endif
{
  if (!m_dll.Load()) return NULL;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return NULL;
  }
#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 pTexture = NULL;
  g_graphicsContext.Get3DDevice()->CreateTexture(m_info.width, m_info.height, 1, 0, D3DFMT_LIN_A8R8G8B8 , D3DPOOL_MANAGED, &pTexture);
#else
  SDL_Surface *pTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, m_info.width, m_info.height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
#endif
  if (pTexture)
  {
    CLog::Log(LOGDEBUG,"PICTURE: loaded image and created texture. height: %u, width: %u", m_info.height, m_info.width);
#ifndef HAS_SDL
    D3DLOCKED_RECT lr;
    if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
    {
      DWORD destPitch = lr.Pitch;
      // CxImage aligns rows to 4 byte boundaries
      DWORD srcPitch = ((m_info.width + 1)* 3 / 4) * 4;
      BYTE *pixels = (BYTE *)lr.pBits;
#else
    if (SDL_LockSurface(pTexture) == 0)
    {
      DWORD destPitch = pTexture->pitch;
      DWORD srcPitch = ((m_info.width + 1)* 3 / 4) * 4; 
      BYTE *pixels = (BYTE *)pTexture->pixels;
#endif
      for (unsigned int y = 0; y < m_info.height; y++)
      {
        BYTE *dst = pixels + y * destPitch;
        BYTE *src = m_info.texture + (m_info.height - 1 - y) * srcPitch;
        BYTE *alpha = m_info.alpha + (m_info.height - 1 - y) * m_info.width;
        for (unsigned int x = 0; x < m_info.width; x++)
        {
          *dst++ = *src++;
          *dst++ = *src++;
          *dst++ = *src++;
          *dst++ = (m_info.alpha) ? *alpha++ : 0xff;  // alpha
        }
      }
  
#ifndef HAS_SDL
      pTexture->UnlockRect( 0 );
#else
      SDL_UnlockSurface(pTexture);
#endif
    }
  }
  m_dll.ReleaseImage(&m_info);
  return pTexture;
}

bool CPicture::DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence /*= false*/)
{
  // don't create the thumb if it already exists
  if (checkExistence && CFile::Exists(strThumbFileName))
    return true;

  CLog::Log(LOGINFO, "Creating thumb from: %s as: %s", strFileName.c_str(),strThumbFileName.c_str());

  // load our dll
  if (!m_dll.Load()) return false;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.CreateThumbnail(strFileName.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    CLog::Log(LOGERROR, "PICTURE::DoCreateThumbnail: Unable to create thumbfile %s from image %s", strThumbFileName.c_str(), strFileName.c_str());
    return false;
  }
  return true;
}

bool CPicture::CreateThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", strThumbFileName.c_str());
  if (!m_dll.Load()) return false;
  if (!m_dll.CreateThumbnailFromMemory((BYTE *)pBuffer, nBufSize, strExtension.c_str(), strThumbFileName.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateAlbumThumbnailFromMemory: exception: memfile FileType: %s\n", strExtension.c_str());
    return false;
  }
  return true;
}

void CPicture::CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail)
{ // we want to mold the thumbs together into one single one
  if (!m_dll.Load()) return;
  CStdString strThumbnails[4];
  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
  {
    if (strThumbs[i].IsEmpty())
      strThumbnails[i] = "";
    else
    {
      CFileItem item(strThumbs[i], false);
      strThumbnails[i] = item.GetCachedPictureThumb();
      DoCreateThumbnail(strThumbs[i], strThumbnails[i], true);
    }
    szThumbs[i] = strThumbnails[i].c_str();
  }
  if (!m_dll.CreateFolderThumbnail(szThumbs, folderThumbnail.c_str(), g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateFolderThumb() failed for folder thumb %s", folderThumbnail.c_str());
  }
}

bool CPicture::CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName)
{
  if (!pBuffer || !m_dll.Load()) return false;
  return m_dll.CreateThumbnailFromSurface(pBuffer, width, height, stride, strThumbFileName.c_str());
}

int CPicture::ConvertFile(const CStdString &srcFile, const CStdString &destFile, float rotateDegrees, int width, int height, unsigned int quality)
{
  if (!m_dll.Load()) return false;
  int ret;
  ret=m_dll.ConvertFile(srcFile.c_str(), destFile.c_str(), rotateDegrees, width, height, quality);
  if (ret!=0)
  {
    CLog::Log(LOGERROR, "PICTURE: Error %i converting image %s", ret, srcFile.c_str());
    return ret;
  }
  return ret;
}

// caches a skin image as a thumbnail image
bool CPicture::CacheSkinImage(const CStdString &srcFile, const CStdString &destFile)
{
  int iImages = g_TextureManager.Load(srcFile, 0);
  if (iImages > 0)
  {
    int width, height;
    bool linear;
#ifndef HAS_SDL
    LPDIRECT3DPALETTE8 palette;
    LPDIRECT3DTEXTURE8 texture = g_TextureManager.GetTexture(srcFile, 0, width, height, palette, linear);
#elif defined(HAS_SDL_2D)
    SDL_Palette* palette;
    SDL_Surface* texture = g_TextureManager.GetTexture(srcFile, 0, width, height, palette, linear);
#elif defined(HAS_SDL_OPENGL)
#ifdef __GNUC__
#warning fix this code to support OpenGL
#endif
    SDL_Surface* texture = NULL;
#endif
    if (texture)
    {
      bool success(false);
      CPicture pic;
      if (!linear)
      { // damn, have to copy it to a linear texture first :(
        return CreateThumbnailFromSwizzledTexture(texture, width, height, destFile);
      }
      else
      {
#ifndef HAS_SDL
        D3DLOCKED_RECT lr;
        texture->LockRect(0, &lr, NULL, 0);
        success = pic.CreateThumbnailFromSurface((BYTE *)lr.pBits, width, height, lr.Pitch, destFile);
        texture->UnlockRect(0);
#else
	SDL_LockSurface(texture);
        success = pic.CreateThumbnailFromSurface((BYTE *)texture->pixels, width, height, texture->pitch, destFile);
	SDL_UnlockSurface(texture);
#endif
      }
      g_TextureManager.ReleaseTexture(srcFile, 0);
      return success;
    }
  }
  return false;
}

#ifndef HAS_SDL
bool CPicture::CreateThumbnailFromSwizzledTexture(LPDIRECT3DTEXTURE8 &texture, int width, int height, const CStdString &thumb)
#else
bool CPicture::CreateThumbnailFromSwizzledTexture(SDL_Surface* &texture, int width, int height, const CStdString &thumb)
#endif
{
#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 linTexture = NULL;
  if (D3D_OK == D3DXCreateTexture(g_graphicsContext.Get3DDevice(), width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &linTexture))
  {
    LPDIRECT3DSURFACE8 source;
    LPDIRECT3DSURFACE8 dest;
    texture->GetSurfaceLevel(0, &source);
    linTexture->GetSurfaceLevel(0, &dest);
    D3DXLoadSurfaceFromSurface(dest, NULL, NULL, source, NULL, NULL, D3DX_FILTER_NONE, 0);
    D3DLOCKED_RECT lr;
    dest->LockRect(&lr, NULL, 0);
    bool success = CreateThumbnailFromSurface((BYTE *)lr.pBits, width, height, lr.Pitch, thumb);
    dest->UnlockRect();
    SAFE_RELEASE(source);
    SAFE_RELEASE(dest);
    SAFE_RELEASE(linTexture);
    return success;
  }
#else
#ifdef __GNUC__
#warning FIXME CPicture::CreateThumbnailFromSwizzledTexture not implemented
#endif
#endif
  return false;
}

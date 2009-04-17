

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

#ifndef HAS_SDL
CTexture::CTexture(int width, int height, int loops, LPDIRECT3DPALETTE8 palette, bool texCoordsArePixels)
#else
CTexture::CTexture(int width, int height, int loops, SDL_Palette* palette, bool texCoordsArePixels)
#endif
{
  m_width = width;
  m_height = height;
  m_loops = loops;
  m_palette = palette;
  m_texWidth = 0;
  m_texHeight = 0;
  m_texCoordsArePixels = false;
};

unsigned int CTexture::size() const
{
  return m_textures.size();
}

#if !defined(HAS_SDL)
void CTexture::Add(LPDIRECT3DTEXTURE8 texture, int delay)
#elif defined(HAS_SDL_2D)
void CTexture::Add(SDL_Surface *texture, int delay)
#else
void CTexture::Add(CGLTexture *texture, int delay)
#endif
{
  if (!texture)
    return;

  m_textures.push_back(texture);
  m_delays.push_back(delay ? delay * 2 : 100);
#ifndef HAS_SDL
  D3DSURFACE_DESC desc;
  if (D3D_OK == texture->GetLevelDesc(0, &desc))
  {
    m_texWidth = desc.Width;
    m_texHeight = desc.Height;
    m_texCoordsArePixels = desc.Format == D3DFMT_LIN_A8R8G8B8;
  }
#elif defined(HAS_SDL_2D)
  m_texWidth = texture->w;
  m_texHeight = texture->h;
  m_texCoordsArePixels = false;
#elif defined(HAS_SDL_OPENGL)
  m_texWidth = texture->textureWidth;
  m_texHeight = texture->textureHeight;
  m_texCoordsArePixels = false;
#endif
}

#if !defined(HAS_SDL)
void CTexture::Set(LPDIRECT3DTEXTURE8 texture, int width, int height)
#elif defined(HAS_SDL_2D)
void CTexture::Set(SDL_Surface *texture, int width, int height)
#else
void CTexture::Set(CGLTexture *texture, int width, int height)
#endif
{
  assert(!m_textures.size()); // don't try and set a texture if we already have one!
  m_width = width;
  m_height = height;
  Add(texture, 100);
}

void CTexture::Free()
{
  CSingleLock lock(g_graphicsContext);
  for (unsigned int i = 0; i < m_textures.size(); i++)
  {
#if defined(HAS_SDL_2D)
    SDL_FreeSurface(m_textures[i]);
#elif defined(HAS_SDL_OPENGL)
    delete m_textures[i];
#else
    m_textures[i]->Release();
#endif
  }
  m_textures.clear();
  m_delays.clear();
  // Note that in SDL and Win32 we already convert the paletted textures into normal textures,
  // so there's no chance of having m_pPalette as a real palette
  m_palette = NULL;

  Reset();
}

CTextureMap::CTextureMap()
{
  m_textureName = "";
  m_referenceCount = 0;
  m_memUsage = 0;
}

#ifndef HAS_SDL
CTextureMap::CTextureMap(const CStdString& textureName, int width, int height, int loops, LPDIRECT3DPALETTE8 *palette)
#else
CTextureMap::CTextureMap(const CStdString& textureName, int width, int height, int loops, SDL_Palette *palette)
#endif
: m_texture(width, height, loops, palette)
{
  m_textureName = textureName;
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::~CTextureMap()
{
  FreeTexture();
}

void CTextureMap::Dump() const
{
  if (!m_referenceCount)
    return;   // nothing to see here

  CStdString strLog;
  strLog.Format("  texture:%s has %i frames %i refcount\n", m_textureName.c_str(), m_texture.m_textures.size(), m_referenceCount);
  OutputDebugString(strLog.c_str());
}

const CStdString& CTextureMap::GetName() const
{
  return m_textureName;
}

#ifndef HAS_SDL
void CTextureMap::Add(LPDIRECT3DTEXTURE8 pTexture, int delay)
#else
void CTextureMap::Add(SDL_Surface* pTexture, int delay)
#endif
{
#ifdef HAS_SDL_OPENGL
  CGLTexture *glTexture = new CGLTexture(pTexture, false);
  m_texture.Add(glTexture, delay);
#else
  m_texture.Add(pTexture, delay);
#endif

#ifndef HAS_SDL
  D3DSURFACE_DESC desc;
  if (pTexture && D3D_OK == pTexture->GetLevelDesc(0, &desc))
    m_memUsage += desc.Size;
#elif defined(HAS_SDL_2D)
  if (pTexture)
    m_memUsage += sizeof(SDL_Surface) + (pTexture->w * pTexture->h * pTexture->format->BytesPerPixel);
#elif defined(HAS_SDL_OPENGL)
  if (glTexture)
    m_memUsage += sizeof(CGLTexture) + (glTexture->textureWidth * glTexture->textureHeight * 4);
#endif
}

bool CTextureMap::Release()
{
  if (!m_texture.m_textures.size()) return true;
  if (!m_referenceCount) return true;

  m_referenceCount--;
  if (!m_referenceCount)
  {
    FreeTexture();
    return true;
  }
  return false;
}

const CTexture &CTextureMap::GetTexture()
{
  m_referenceCount++;
  return m_texture;
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

DWORD CTextureMap::GetMemoryUsage() const
{
  return m_memUsage;
}

bool CTextureMap::IsEmpty() const
{
  return m_texture.m_textures.size() == 0;
}

//------------------------------------------------------------------------------
CGUITextureManager::CGUITextureManager(void)
{
  for (int bundle = 0; bundle < 2; bundle++)
    m_iNextPreload[bundle] = m_PreLoadNames[bundle].end();
  // we set the theme bundle to be the first bundle (thus prioritising it
  m_TexBundle[0].SetThemeBundle(true);

#if defined(HAS_SDL) && defined(_WIN32)
  // Hack for SDL library that keeps loading and unloading these
  LoadLibraryEx("zlib1.dll", NULL, 0);
  LoadLibraryEx("libpng12-0.dll", NULL, 0);
#endif

}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}

static const CTexture emptyTexture;

const CTexture &CGUITextureManager::GetTexture(const CStdString& strTextureName)
{
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

#ifndef HAS_SDL
// Round a number to the nearest power of 2 rounding up
// runs pretty quickly - the only expensive op is the bsr
// alternive would be to dec the source, round down and double the result
// which is slightly faster but rounds 1 to 2
DWORD __forceinline __stdcall PadPow2(DWORD x)
{
  __asm {
    mov edx, x    // put the value in edx
    xor ecx, ecx  // clear ecx - if x is 0 bsr doesn't alter it
    bsr ecx, edx  // find MSB position
    mov eax, 1    // shift 1 by result effectively
    shl eax, cl   // doing a round down to power of 2
    cmp eax, edx  // check if x was already a power of two
    adc ecx, 0    // if it wasn't then CF is set so add to ecx
    mov eax, 1    // shift 1 by result again, this does a round
    shl eax, cl   // up as a result of adding CF to ecx
  }
  // return result in eax
}
#elif defined(HAS_SDL_2D)
// SDL does not care about the surfaces being a power of 2
DWORD PadPow2(DWORD x)
{
  return x;
}
#elif defined(HAS_SDL_OPENGL)
DWORD PadPow2(DWORD x)
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
#endif

void CGUITextureManager::StartPreLoad()
{
  for (int bundle = 0; bundle < 2; bundle++)
    m_PreLoadNames[bundle].clear();
}

void CGUITextureManager::PreLoad(const CStdString& strTextureName)
{
  if (strTextureName.c_str()[1] == ':' || strTextureName == "-")
    return ;

  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
      return ;
  }

  for (int bundle = 0; bundle < 2; bundle++)
  {
    for (list<CStdString>::iterator i = m_PreLoadNames[bundle].begin(); i != m_PreLoadNames[bundle].end(); ++i)
    {
      if (*i == strTextureName)
        return ;
    }

    if (m_TexBundle[bundle].HasFile(strTextureName))
    {
      m_PreLoadNames[bundle].push_back(strTextureName);
      return;
    }
  }
}

void CGUITextureManager::EndPreLoad()
{
  for (int i = 0; i < 2; i++)
  {
    m_iNextPreload[i] = m_PreLoadNames[i].begin();
    // preload next file
    if (m_iNextPreload[i] != m_PreLoadNames[i].end())
      m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
  }
}

void CGUITextureManager::FlushPreLoad()
{
  for (int i = 0; i < 2; i++)
  {
    m_PreLoadNames[i].clear();
    m_iNextPreload[i] = m_PreLoadNames[i].end();
  }
}

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
      for (int i = 0; i < 2; i++)
      {
        if (m_iNextPreload[i] != m_PreLoadNames[i].end() && (*m_iNextPreload[i] == bundledName))
        {
          ++m_iNextPreload[i];
          // preload next file
          if (m_iNextPreload[i] != m_PreLoadNames[i].end())
            m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
        }
      }
      if (size) *size = 1;
      return true;
    }
  }

  for (int i = 0; i < 2; i++)
  {
    if (m_iNextPreload[i] != m_PreLoadNames[i].end() && (*m_iNextPreload[i] == bundledName))
    {
      if (bundle) *bundle = i;
      ++m_iNextPreload[i];
      // preload next file
      if (m_iNextPreload[i] != m_PreLoadNames[i].end())
        m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
      return true;
    }
    else if (m_TexBundle[i].HasFile(bundledName))
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

#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 pTexture;
  LPDIRECT3DPALETTE8 pPal = 0;
#else
  SDL_Surface* pTexture;
  SDL_Palette* pPal = NULL;
#endif

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
#ifndef HAS_SDL
      LPDIRECT3DTEXTURE8* pTextures;
#else
      SDL_Surface** pTextures;
#endif
      int nLoops = 0;
      int* Delay;
#ifndef HAS_SDL
      int nImages = m_TexBundle[bundle].LoadAnim(g_graphicsContext.Get3DDevice(), strTextureName, &info, &pTextures, &pPal, nLoops, &Delay);
#else
      int nImages = m_TexBundle[bundle].LoadAnim(strTextureName, &info, &pTextures, &pPal, nLoops, &Delay);
#endif
      if (!nImages)
      {
        CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
        return 0;
      }

      pMap = new CTextureMap(strTextureName, info.Width, info.Height, nLoops, pPal);
      for (int iImage = 0; iImage < nImages; ++iImage)
      {
        pMap->Add(pTextures[iImage], Delay[iImage]);
#ifndef HAS_SDL
        delete pTextures[iImage];
#elif defined(HAS_SDL_OPENGL)
        SDL_FreeSurface(pTextures[iImage]);
#endif
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
      pMap = new CTextureMap(strTextureName, iWidth, iHeight, AnimatedGifSet.nLoops, pPal);

      for (int iImage = 0; iImage < iImages; iImage++)
      {
        int w = iWidth;
        int h = iHeight;

#if defined(HAS_SDL)
        pTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 32, RMASK, GMASK, BMASK, AMASK);
        if (pTexture)
#else
        if (D3DXCreateTexture(g_graphicsContext.Get3DDevice(), w, h, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &pTexture) == D3D_OK)
#endif
        {
          CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];
#ifndef HAS_SDL
          D3DLOCKED_RECT lr;
          RECT rc = { 0, 0, pImage->Width, pImage->Height };
          if ( D3D_OK == pTexture->LockRect( 0, &lr, &rc, 0 ))
#else
          if (SDL_LockSurface(pTexture) != -1)
#endif
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
#ifndef HAS_SDL
              BYTE *dest = (BYTE *)lr.pBits + y * lr.Pitch;
#else
              BYTE *dest = (BYTE *)pTexture->pixels + (y * w * 4);
#endif
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

#ifndef HAS_SDL
            pTexture->UnlockRect( 0 );
#else
            SDL_UnlockSurface(pTexture);
#endif

            pMap->Add(pTexture, pImage->Delay);

#ifdef HAS_SDL_OPENGL
            SDL_FreeSurface(pTexture);
#endif
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
#ifndef HAS_SDL
    if (FAILED(m_TexBundle[bundle].LoadTexture(g_graphicsContext.Get3DDevice(), strTextureName, &info, &pTexture, &pPal)))
#else
    if (FAILED(m_TexBundle[bundle].LoadTexture(strTextureName, &info, &pTexture, &pPal)))
#endif
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

#ifndef HAS_SDL
    if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), _P(texturePath).c_str(),
                                      D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
                                      D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
    {
      if (!strnicmp(strPath.c_str(), "special://home/skin/", 20) && !strnicmp(strPath.c_str(), "special://xbmc/skin/", 20))
        CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
      return 0;

    }
#else
    SDL_Surface *original = IMG_Load(_P(texturePath).c_str());
    CPicture pic;
    if (!original && !(original = pic.Load(texturePath, MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT)))
    {
        CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return 0;
    }
    // make sure the texture format is correct
    SDL_PixelFormat format;
    format.palette = 0; format.colorkey = 0; format.alpha = 0;
    format.BitsPerPixel = 32; format.BytesPerPixel = 4;
    format.Amask = AMASK; format.Ashift = PIXEL_ASHIFT;
    format.Rmask = RMASK; format.Rshift = PIXEL_RSHIFT;
    format.Gmask = GMASK; format.Gshift = PIXEL_GSHIFT;
    format.Bmask = BMASK; format.Bshift = PIXEL_BSHIFT;
#ifdef HAS_SDL_OPENGL
    pTexture = SDL_ConvertSurface(original, &format, SDL_SWSURFACE);
#else
    pTexture = SDL_ConvertSurface(original, &format, SDL_HWSURFACE);
#endif
    SDL_FreeSurface(original);
    if (!pTexture)
    {
      CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
      return 0;
    }
    info.Width = pTexture->w;
    info.Height = pTexture->h;
#endif
  }

  CTextureMap* pMap = new CTextureMap(strTextureName, info.Width, info.Height, 0, pPal);
  pMap->Add(pTexture, 100);
  m_vecTextures.push_back(pMap);

#ifdef HAS_SDL_OPENGL
  SDL_FreeSurface(pTexture);
#endif

#ifdef _DEBUG
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

#ifndef _LINUX
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  DWORD dwMegFree = stat.dwAvailPhys / (1024 * 1024);
  if (dwMegFree > 29)
  {
    // dont release skin textures, they are reloaded each time
    //if (strTextureName.GetAt(1) != ':') return;
    //CLog::Log(LOGINFO, "release:%s", strTextureName.c_str());
  }
#endif

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

#ifdef HAS_SDL_OPENGL
CGLTexture::CGLTexture(SDL_Surface* surface, bool load, bool freeSurface)
{
  m_loadedToGPU = false;
  id = 0;
  m_pixels = NULL;

  Update(surface, load, freeSurface);

}

void CGLTexture::LoadToGPU()
{
  if (!m_pixels) {
    // nothing to load - probably same image (no change)
    return;
  }

  g_graphicsContext.BeginPaint();
  if (!m_loadedToGPU) {
     // Have OpenGL generate a texture object handle for us
     // this happens only one time - the first time the texture is loaded
     glGenTextures(1, &id);
  }

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, id);

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  static GLint maxSize = g_graphicsContext.GetMaxTextureSize();
  {
    if (textureHeight>maxSize)
    {
      CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %d", textureHeight, maxSize);
      textureHeight = maxSize;
    }
    if (textureWidth>maxSize)
    {
      CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %d", textureWidth, maxSize);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
      textureWidth = maxSize;
    }
  }
  //CLog::Log(LOGNOTICE, "Texture width x height: %d x %d", textureWidth, textureHeight);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, textureWidth, textureHeight, 0,
               GL_BGRA, GL_UNSIGNED_BYTE, m_pixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  VerifyGLState();

  g_graphicsContext.EndPaint();
  delete [] m_pixels;
  m_pixels = NULL;

  m_loadedToGPU = true;
}

void CGLTexture::Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU) {
  static int vmaj=0;
  int vmin,tpitch;

   if (m_pixels)
      delete [] m_pixels;

  imageWidth = w;
  imageHeight = h;


  if ((vmaj==0) && g_graphicsContext.getScreenSurface())
  {
    g_graphicsContext.getScreenSurface()->GetGLVersion(vmaj, vmin);
  }
  if (vmaj>=2 && GLEW_ARB_texture_non_power_of_two)
  {
    textureWidth = imageWidth;
    textureHeight = imageHeight;
  }
  else
  {
    textureWidth = PadPow2(imageWidth);
    textureHeight = PadPow2(imageHeight);
  }

  // Resize texture to POT
  const unsigned char *src = pixels;
  tpitch = min(pitch,textureWidth*4);
  m_pixels = new unsigned char[textureWidth * textureHeight * 4];
  unsigned char* resized = m_pixels;

  for (int y = 0; y < h; y++)
  {
    memcpy(resized, src, tpitch);  // make sure pitch is not bigger than our width
    src += pitch;

    // repeat last column to simulate clamp_to_edge
    for(int i = tpitch; i < textureWidth*4; i+=4)
      memcpy(resized+i, src-4, 4);

    resized += (textureWidth * 4);
  }

  // repeat last row to simulate clamp_to_edge
  for(int y = h; y < textureHeight; y++)
  {
    memcpy(resized, src - tpitch, tpitch);

    // repeat last column to simulate clamp_to_edge
    for(int i = tpitch; i < textureWidth*4; i+=4)
      memcpy(resized+i, src-4, 4);

    resized += (textureWidth * 4);
  }
  if (loadToGPU)
     LoadToGPU();
}

void CGLTexture::Update(SDL_Surface *surface, bool loadToGPU, bool freeSurface) {

  SDL_LockSurface(surface);
  Update(surface->w, surface->h, surface->pitch, (unsigned char *)surface->pixels, loadToGPU);
  SDL_UnlockSurface(surface);

  if (freeSurface)
    SDL_FreeSurface(surface);
}

CGLTexture::~CGLTexture()
{
  g_graphicsContext.BeginPaint();
  if (glIsTexture(id)) {
      glDeleteTextures(1, &id);
  }
  g_graphicsContext.EndPaint();

  if (m_pixels)
     delete [] m_pixels;

  m_pixels=NULL;
  id = 0;
}
#endif

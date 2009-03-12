

#include "include.h"
#include "TextureManager.h"
#include "AnimatedGif.h"
#include "PackedTexture.h"
#include "GraphicContext.h"
#include "utils/SingleLock.h"
#include "StringUtils.h"
#include "utils/CharsetConverter.h"
#include "../xbmc/Util.h"
#include "../xbmc/FileSystem/File.h"
#include "../xbmc/FileSystem/Directory.h"
#include "../xbmc/FileSystem/SpecialProtocol.h"

#ifdef HAS_XBOX_D3D
#include <XGraphics.h>
#endif

using namespace std;

extern "C" void dllprintf( const char *format, ... );

CGUITextureManager g_TextureManager;

CTexture::CTexture(int width, int height, int loops, LPDIRECT3DPALETTE8 palette, bool packed, bool texCoordsArePixels)
{
  m_width = width;
  m_height = height;
  m_loops = loops;
  m_palette = palette;
  m_texWidth = 0;
  m_texHeight = 0;
#ifdef HAS_XBOX_D3D
  m_texCoordsArePixels = texCoordsArePixels;
  if (m_palette)
    m_palette->AddRef();
#else
  m_texCoordsArePixels = false;
#endif
  m_packed = packed;
};

unsigned int CTexture::size() const
{
  return m_textures.size();
}

void CTexture::Add(LPDIRECT3DTEXTURE8 texture, int delay)
{
  if (!texture)
    return;

  m_textures.push_back(texture);
  m_delays.push_back(delay ? delay * 2 : 100);
  D3DSURFACE_DESC desc;
  if (D3D_OK == texture->GetLevelDesc(0, &desc))
  {
    m_texWidth = desc.Width;
    m_texHeight = desc.Height;
#ifdef HAS_XBOX_D3D
    m_texCoordsArePixels = desc.Format == D3DFMT_LIN_A8R8G8B8;
#endif
  }
}

void CTexture::Set(LPDIRECT3DTEXTURE8 texture, int width, int height)
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
    if (m_packed)
    {
#ifdef HAS_XBOX_D3D
      m_textures[i]->BlockUntilNotBusy();
      void* Data = (void*)(*(DWORD*)(((char*)m_textures[i]) + sizeof(D3DTexture)));
      if (Data)
        XPhysicalFree(Data);
      delete [] m_textures[i];
#else
      m_textures[i]->Release();
#endif
    }
    else
      m_textures[i]->Release();
  }
  m_textures.clear();
  m_delays.clear();
  // Note that in SDL and Win32 we already convert the paletted textures into normal textures, 
  // so there's no chance of having m_palette as a real palette
#ifdef HAS_XBOX_D3D
  if (m_palette)
  {
    if (m_packed)
    {
      if ((m_palette->Common & D3DCOMMON_REFCOUNT_MASK) > 1)
        m_palette->Release();
      else
        delete m_palette;
    }
    else
      m_palette->Release();
  }
#endif
  m_palette = NULL;

  Reset();
}

CTextureMap::CTextureMap()
{
  m_textureName = "";
  m_referenceCount = 0;
  m_memUsage = 0;
}

CTextureMap::CTextureMap(const CStdString& textureName, int width, int height, int loops, LPDIRECT3DPALETTE8 palette, bool packed)
: m_texture(width, height, loops, palette, packed)
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

  D3DSURFACE_DESC desc;
  if (pTexture && D3D_OK == pTexture->GetLevelDesc(0, &desc))
    m_memUsage += desc.Size;
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
#ifdef HAS_XBOX_D3D
  D3DXSetDXT3DXT5(TRUE);
#endif
  for (int bundle = 0; bundle < 2; bundle++)
    m_iNextPreload[bundle] = m_PreLoadNames[bundle].end();
  // we set the theme bundle to be the first bundle (thus prioritising it
  m_TexBundle[0].SetThemeBundle(true);
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

  LPDIRECT3DTEXTURE8 pTexture;
  LPDIRECT3DPALETTE8 pPal = 0;

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
      LPDIRECT3DTEXTURE8* pTextures;
      int nLoops = 0;
      int* Delay;
      int nImages = m_TexBundle[bundle].LoadAnim(g_graphicsContext.Get3DDevice(), strTextureName, &info, &pTextures, &pPal, nLoops, &Delay);
      if (!nImages)
      {
        CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
        return 0;
      }

      pMap = new CTextureMap(strTextureName, info.Width, info.Height, nLoops, pPal, true);
      for (int iImage = 0; iImage < nImages; ++iImage)
      {
        pMap->Add(pTextures[iImage], Delay[iImage]);
      }

      delete [] pTextures;
      delete [] Delay;
#ifdef HAS_XBOX_D3D
      pPal->Release();
#endif
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
#ifdef HAS_XBOX_D3D
      g_graphicsContext.Get3DDevice()->CreatePalette(D3DPALETTE_256, &pPal);
      PALETTEENTRY* pal;
      pPal->Lock((D3DCOLOR**)&pal, 0);

      memcpy(pal, AnimatedGifSet.m_vecimg[0]->Palette, sizeof(PALETTEENTRY)*iPaletteSize);
      for (int i = 0; i < iPaletteSize; i++)
        pal[i].peFlags = 0xff; // alpha
      if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
        pal[AnimatedGifSet.m_vecimg[0]->Transparent].peFlags = 0;

      pPal->Unlock();
#endif
      pMap = new CTextureMap(strTextureName, iWidth, iHeight, AnimatedGifSet.nLoops, pPal, false);
      for (int iImage = 0; iImage < iImages; iImage++)
      {
        int w = PadPow2(iWidth);
        int h = PadPow2(iHeight);
#ifdef HAS_XBOX_D3D
        if (D3DXCreateTexture(g_graphicsContext.Get3DDevice(), w, h, 1, 0, D3DFMT_P8, D3DPOOL_MANAGED, &pTexture) == D3D_OK)
#else
        if (D3DXCreateTexture(g_graphicsContext.Get3DDevice(), w, h, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED, &pTexture) == D3D_OK)
#endif
        {
          CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];
          D3DLOCKED_RECT lr;
          RECT rc = { 0, 0, pImage->Width, pImage->Height };
          if ( D3D_OK == pTexture->LockRect( 0, &lr, &rc, 0 ))
          {
#ifdef HAS_XBOX_D3D
            POINT pt = { 0, 0 };
            XGSwizzleRect(pImage->Raster, pImage->BytesPerRow, &rc, lr.pBits, w, h, &pt, 1);
#else
            COLOR *palette = AnimatedGifSet.m_vecimg[0]->Palette;
            // set the alpha values to fully opaque
            for (int i = 0; i < iPaletteSize; i++)
              palette[i].x = 0xff;
            // and set the transparent colour
            if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
              palette[AnimatedGifSet.m_vecimg[0]->Transparent].x = 0;
            
            for (int y = 0; y < pImage->Height; y++)
            {
              BYTE *dest = (BYTE *)lr.pBits + y * lr.Pitch;
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
#endif
            pTexture->UnlockRect( 0 );

            pMap->Add(pTexture, pImage->Delay);
          }
        }
      } // of for (int iImage=0; iImage < iImages; iImage++)

#ifdef HAS_XBOX_D3D
      pPal->Release();
#endif
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
    if (FAILED(m_TexBundle[bundle].LoadTexture(g_graphicsContext.Get3DDevice(), strTextureName, &info, &pTexture, &pPal)))
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

    if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), _P(texturePath).c_str(),
                                     D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
                                     D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, &info, NULL, &pTexture) != D3D_OK)
    {
      if (!strnicmp(strPath.c_str(), "special://home/skin/", 20) && !strnicmp(strPath.c_str(), "special://xbmc/skin/", 20))
        CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
      return 0;

    }
  }

  CTextureMap* pMap = new CTextureMap(strTextureName, info.Width, info.Height, 0, pPal, bundle >= 0);
  pMap->Add(pTexture, 100);
  m_vecTextures.push_back(pMap);

#ifdef _DEBUG
  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  char temp[200];
  sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, (bundle >= 0) ? " (bundled)" : "");
  OutputDebugString(temp);
#endif

#ifdef HAS_XBOX_D3D
  if (pPal)
    pPal->Release();
#endif
  return 1;
}

void CGUITextureManager::ReleaseTexture(const CStdString& strTextureName)
{
  CSingleLock lock(g_graphicsContext);

  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  DWORD dwMegFree = stat.dwAvailPhys / (1024 * 1024);
  if (dwMegFree > 29)
  {
    // dont release skin textures, they are reloaded each time
    //if (strTextureName.GetAt(1) != ':') return;
    //CLog::Log(LOGINFO, "release:%s", strTextureName.c_str());
  }


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


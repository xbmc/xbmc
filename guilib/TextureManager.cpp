#include "include.h"
#include "TextureManager.h"
#include "AnimatedGif.h"
#include "PackedTexture.h"
#include "GraphicContext.h"
#include "../xbmc/Picture.h"
#include "../xbmc/utils/SingleLock.h"
#include "../xbmc/StringUtils.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/Util.h"

#ifdef HAS_XBOX_D3D
#include <XGraphics.h>
#endif

#ifdef HAS_SDL
#define MAX_PICTURE_WIDTH  4096
#define MAX_PICTURE_HEIGHT 4096
#endif


extern "C" void dllprintf( const char *format, ... );

CGUITextureManager g_TextureManager;

CTexture::CTexture()
{
  m_iReferenceCount = 0;
  m_pTexture = NULL;
  m_iDelay = 100;
  m_iWidth = m_iHeight = 0;
  m_iLoops = 0;
  m_pPalette = NULL;
  m_bPacked = false;
  m_memUsage = 0;
  m_format = D3DFMT_UNKNOWN;
}

#ifndef HAS_SDL
CTexture::CTexture(LPDIRECT3DTEXTURE8 pTexture, int iWidth, int iHeight, bool bPacked, int iDelay, LPDIRECT3DPALETTE8 pPalette)
#else
CTexture::CTexture(SDL_Surface* pTexture, int iWidth, int iHeight, bool bPacked, int iDelay, SDL_Palette* pPalette)
#endif
{
  m_iLoops = 0;
  m_iReferenceCount = 0;
#ifndef HAS_SDL_OPENGL
  m_pTexture = pTexture;
#else
  m_pTexture = new CGLTexture(pTexture, false);
#endif
  m_iDelay = 2 * iDelay;
  m_iWidth = iWidth;
  m_iHeight = iHeight;
#ifdef HAS_XBOX_D3D
  m_pPalette = pPalette;
  if (m_pPalette)
    m_pPalette->AddRef();
#endif
  m_bPacked = bPacked;
  ReadTextureInfo();
}

CTexture::~CTexture()
{
  FreeTexture();
}

void CTexture::FreeTexture()
{
  CSingleLock lock(g_graphicsContext);

  if (m_pTexture)
  {
    if (m_bPacked)
    {
#ifdef HAS_XBOX_D3D
      m_pTexture->BlockUntilNotBusy();
      void* Data = (void*)(*(DWORD*)(((char*)m_pTexture) + sizeof(D3DTexture)));
      if (Data)
        XPhysicalFree(Data);
      delete [] m_pTexture;
#elif defined(HAS_SDL_2D)
      SDL_FreeSurface(m_pTexture);
#elif defined(HAS_SDL_OPENGL)
      delete m_pTexture;
#else
      m_pTexture->Release();
#endif
    }
    else
#ifdef HAS_SDL_2D
      SDL_FreeSurface(m_pTexture);
#elif defined(HAS_SDL_OPENGL)
      delete m_pTexture;
#else
      m_pTexture->Release();
#endif
    m_pTexture = NULL;
  }
  
// Note that in SDL and Win32 we already conver the paletted textures into normal textures, 
// so there's no chance of having m_pPalette as a real palette
#ifdef HAS_XBOX_D3D
  if (m_pPalette)
  {
    if (m_bPacked)
    {
      if ((m_pPalette->Common & D3DCOMMON_REFCOUNT_MASK) > 1)
        m_pPalette->Release();
      else
        delete m_pPalette;
    }
    else
      m_pPalette->Release();
  }
#endif
  m_pPalette = NULL;
}

void CTexture::Dump() const
{
  if (!m_iReferenceCount) return ;
  CStdString strLog;
  strLog.Format("refcount:%i\n:", m_iReferenceCount);
  OutputDebugString(strLog.c_str());
}

void CTexture::SetLoops(int iLoops)
{
  m_iLoops = iLoops;
}

int CTexture::GetLoops() const
{
  return m_iLoops;
}

void CTexture::SetDelay(int iDelay)
{
  if (iDelay)
  {
    m_iDelay = 2 * iDelay;
  }
  else
  {
    m_iDelay = 100;
  }
}
void CTexture::Flush()
{
  if (!m_iReferenceCount)
    FreeTexture();
}

bool CTexture::Release()
{
  if (!m_pTexture) return true;
  if (!m_iReferenceCount) return true;
  if (m_iReferenceCount > 0)
  {
    m_iReferenceCount--;
  }

  if (!m_iReferenceCount)
  {
    FreeTexture();
    return true;
  }
  return false;
}

int CTexture::GetDelay() const
{
  return m_iDelay;
}

int CTexture::GetRef() const
{
  return m_iReferenceCount;
}

void CTexture::ReadTextureInfo()
{
  m_memUsage = 0;
#ifndef HAS_SDL  
  D3DSURFACE_DESC desc;
  if (m_pTexture && D3D_OK == m_pTexture->GetLevelDesc(0, &desc))
  {
    m_memUsage += desc.Size;
    m_format = desc.Format;
  }
#elif defined(HAS_SDL_2D)
  if (m_pTexture)
  {
    m_memUsage += sizeof(SDL_Surface) + (m_pTexture->w * m_pTexture->h * m_pTexture->format->BytesPerPixel);
    m_format = D3DFMT_A8R8G8B8;
  }
#elif defined(HAS_SDL_OPENGL)
  if (m_pTexture)
  {
    m_memUsage += sizeof(CGLTexture) + (m_pTexture->textureWidth * m_pTexture->textureHeight * 4);
    m_format = D3DFMT_A8R8G8B8;
  }
#endif  
  // palette as well? if (m_pPalette)
/*
  if (m_pPalette)
  {
    D3DPALETTESIZE size = m_pPalette->GetSize();
    switch size
  }*/
}

DWORD CTexture::GetMemoryUsage() const
{
  return m_memUsage;
}

#ifndef HAS_SDL
LPDIRECT3DTEXTURE8 CTexture::GetTexture(int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal, bool &linearTexture)
#elif defined(HAS_SDL_2D)
SDL_Surface* CTexture::GetTexture(int& iWidth, int& iHeight, SDL_Palette*& pPal, bool &linearTexture)
#elif defined(HAS_SDL_OPENGL)
CGLTexture* CTexture::GetTexture(int& iWidth, int& iHeight, SDL_Palette*& pPal, bool &linearTexture)
#endif
{
  if (!m_pTexture) return NULL;
  m_iReferenceCount++;
  iWidth = m_iWidth;
  iHeight = m_iHeight;
  pPal = m_pPalette;
  linearTexture = (m_format == D3DFMT_LIN_A8R8G8B8);
  return m_pTexture;
}

//-----------------------------------------------------------------------------
CTextureMap::CTextureMap()
{
  m_strTextureName = "";
}

CTextureMap::CTextureMap(const CStdString& strTextureName)
{
  m_strTextureName = strTextureName;
}

CTextureMap::~CTextureMap()
{
  for (int i = 0; i < (int)m_vecTexures.size(); ++i)
  {
    CTexture* pTexture = m_vecTexures[i];
    delete pTexture ;
  }
  m_vecTexures.erase(m_vecTexures.begin(), m_vecTexures.end());
}

void CTextureMap::Dump() const
{
  if (IsEmpty()) return ;
  CStdString strLog;
  strLog.Format("  texure:%s has %i frames\n", m_strTextureName.c_str(), m_vecTexures.size());
  OutputDebugString(strLog.c_str());

  for (int i = 0; i < (int)m_vecTexures.size(); ++i)
  {
    const CTexture* pTexture = m_vecTexures[i];

    strLog.Format("    item:%i ", i);
    OutputDebugString(strLog.c_str());
    pTexture->Dump();
  }
}


const CStdString& CTextureMap:: GetName() const
{
  return m_strTextureName;
}

int CTextureMap::size() const
{
  return m_vecTexures.size();
}

bool CTextureMap::IsEmpty() const
{
  int iRef = 0;
  for (int i = 0; i < (int)m_vecTexures.size(); ++i)
  {
    iRef += m_vecTexures[i]->GetRef();
  }
  return (iRef == 0);
}

void CTextureMap::Add(CTexture* pTexture)
{
  m_vecTexures.push_back(pTexture);
}

bool CTextureMap::Release(int iPicture)
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return true;

  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->Release();
}

int CTextureMap::GetLoops(int iPicture) const
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return 0;
  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->GetLoops();
}

int CTextureMap::GetDelay(int iPicture) const
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return 100;

  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->GetDelay();
}

#ifndef HAS_SDL
LPDIRECT3DTEXTURE8 CTextureMap::GetTexture(int iPicture, int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal, bool &linearTexture)
#elif defined(HAS_SDL_2D)
SDL_Surface* CTextureMap::GetTexture(int iPicture, int& iWidth, int& iHeight, SDL_Palette*& pPal, bool &linearTexture)
#elif defined(HAS_SDL_OPENGL)
CGLTexture* CTextureMap::GetTexture(int iPicture, int& iWidth, int& iHeight, SDL_Palette*& pPal, bool &linearTexture)
#endif
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return NULL;

  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->GetTexture(iWidth, iHeight, pPal, linearTexture);
}

void CTextureMap::Flush()
{
  for (int i = 0; i < (int)m_vecTexures.size(); ++i)
  {
    m_vecTexures[i]->Flush();
  }
}

DWORD CTextureMap::GetMemoryUsage() const
{
  DWORD memUsage = 0;
  for (int i = 0; i < (int)m_vecTexures.size(); ++i)
  {
    memUsage += m_vecTexures[i]->GetMemoryUsage();
  }
  return memUsage;
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

#ifndef HAS_SDL  
LPDIRECT3DTEXTURE8 CGUITextureManager::GetTexture(const CStdString& strTextureName, int iItem, int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal, bool &linearTexture)
#elif defined(HAS_SDL_2D)
SDL_Surface* CGUITextureManager::GetTexture(const CStdString& strTextureName, int iItem, int& iWidth, int& iHeight, SDL_Palette*& pPal, bool &linearTexture)
#else 
CGLTexture* CGUITextureManager::GetTexture(const CStdString& strTextureName, int iItem, int& iWidth, int& iHeight, SDL_Palette*& pPal, bool &linearTexture)  
#endif
{
  //  CLog::Log(LOGINFO, " refcount++ for  GetTexture(%s)\n", strTextureName.c_str());
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      //CLog::Log(LOGDEBUG, "Total memusage %u", GetMemoryUsage());
      return pMap->GetTexture(iItem, iWidth, iHeight, pPal, linearTexture);
    }
  }
  return NULL;
}

int CGUITextureManager::GetLoops(const CStdString& strTextureName, int iPicture) const
{
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return pMap->GetLoops(iPicture);
    }
  }
  return 0;
}
int CGUITextureManager::GetDelay(const CStdString& strTextureName, int iPicture) const
{
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return pMap->GetDelay(iPicture);
    }
  }
  return 100;
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

int CGUITextureManager::Load(const CStdString& strTextureName, DWORD dwColorKey, bool checkBundleOnly /*= false */)
{
  if (strTextureName == "-")
    return 0;
  if (strTextureName.Find("://") >= 0)
    return 0;

  // first check of texture exists...
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      for (int i = 0; i < 2; i++)
      {
        if (m_iNextPreload[i] != m_PreLoadNames[i].end() && (*m_iNextPreload[i] == strTextureName))
        {
          ++m_iNextPreload[i];
          // preload next file
          if (m_iNextPreload[i] != m_PreLoadNames[i].end())
            m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
        }
      }
      return pMap->size();
    }
  }

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(g_graphicsContext);

#ifdef ALLOW_TEXTURE_COMPRESSION
#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 pTexture;
  LPDIRECT3DPALETTE8 pPal = 0;
#else
  SDL_Surface* pTexture;
  SDL_Palette* pPal = NULL;
#endif
  int bundle = -1;
  const CStdString* pstrBundleTex = NULL;
  for (int i = 0; i < 2; i++)
  {
    if (m_iNextPreload[i] != m_PreLoadNames[i].end() && (*m_iNextPreload[i] == strTextureName)) // || *m_iNextPreload == strPalTex))
    {
      pstrBundleTex = &strTextureName;

      bundle = i;
      ++m_iNextPreload[i];
      // preload next file
      if (m_iNextPreload[i] != m_PreLoadNames[i].end())
        m_TexBundle[i].PreloadFile(*m_iNextPreload[i]);
      break;
    }
    else if (m_TexBundle[i].HasFile(strTextureName))
    {
      pstrBundleTex = &strTextureName;
      bundle = i;
      break;
    }
  }

  CStdString strPath;

  if (checkBundleOnly && bundle == -1)
    return 0;

  if (bundle == -1)
    strPath = GetTexturePath(strTextureName);
  else
    strPath = strTextureName;

#ifdef _DEBUG
  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);
#endif

  bool bPacked;
  CStdString strPackedPath;
  if (bundle == -1)
  {
    strPackedPath = strPath;
    strPackedPath += ".xpr";
#ifndef _LINUX
    bPacked = (GetFileAttributes(strPackedPath.c_str()) != -1);
#else
	 struct stat statBuf;
	 bPacked = (stat(strPackedPath.c_str(), &statBuf) != -1);
#endif
  }
  else
    bPacked = false;

  D3DXIMAGE_INFO info;

  if (strPath.Right(4).ToLower() == ".gif")
  {
    CTextureMap* pMap;

    if (bPacked || bundle >= 0)
    {
#ifndef HAS_SDL    
      LPDIRECT3DTEXTURE8* pTextures;
#else
	   SDL_Surface** pTextures;
#endif	   
      int nLoops = 0;
      int* Delay;
      int nImages;
      if (bundle >= 0)
      {
#ifndef HAS_SDL
        nImages = m_TexBundle[bundle].LoadAnim(g_graphicsContext.Get3DDevice(), *pstrBundleTex, &info, &pTextures, &pPal, nLoops, &Delay);
#else
        nImages = m_TexBundle[bundle].LoadAnim(*pstrBundleTex, &info, &pTextures, &pPal, nLoops, &Delay);
#endif        
        if (!nImages)
        {
          CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", pstrBundleTex->c_str());
          return 0;
        }
      }
      else // packed
      {
#ifdef HAS_XBOX_D3D
        nImages = LoadPackedAnim(g_graphicsContext.Get3DDevice(), strPackedPath.c_str(), &info, &pTextures, &pPal, nLoops, &Delay);
        if (!nImages)
#endif
        {
          if (!strnicmp(strPackedPath.c_str(), "q:\\skin", 7))
            CLog::Log(LOGERROR, "Texture manager unable to load packed file: %s", strPackedPath.c_str());
          return 0;
        }
      }

      pMap = new CTextureMap(strTextureName);
      for (int iImage = 0; iImage < nImages; ++iImage)
      {
        CTexture* pclsTexture = new CTexture(pTextures[iImage], info.Width, info.Height, true, 100, pPal);
        pclsTexture->SetDelay(Delay[iImage]);
        pclsTexture->SetLoops(nLoops);
        pMap->Add(pclsTexture);
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
        if (!strnicmp(strPath.c_str(), "q:\\skin", 7))
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
      pMap = new CTextureMap(strTextureName);
      static int npot = -1;
#ifdef HAS_SDL_OPENGL
      if ((npot==-1) && g_graphicsContext.getScreenSurface())
      {
        int vmaj,vmin;
        g_graphicsContext.getScreenSurface()->GetGLVersion(vmaj, vmin);    
        if (vmaj>=2 && GLEW_ARB_texture_non_power_of_two)
          npot = 1;
        else
          npot = 0;
      }
#endif
      for (int iImage = 0; iImage < iImages; iImage++)
      {
        int w;
        int h;
        if (npot==1)
        {
          w = iWidth;
          h = iHeight;
        }
        else
        {
          w = PadPow2(iWidth);
          h = PadPow2(iHeight);
        }
#ifdef HAS_XBOX_D3D
        if (D3DXCreateTexture(g_graphicsContext.Get3DDevice(), w, h, 1, 0, D3DFMT_P8, D3DPOOL_MANAGED, &pTexture) == D3D_OK)
#elif defined(HAS_SDL)
          pTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
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
            
#ifdef HAS_SDL
				    // Allocate memory for the actual pixels in the surface and set the surface
				    BYTE* pixels = (BYTE*) malloc(w * h * 4);
				    pTexture->pixels = pixels;
#endif            
            for (int y = 0; y < pImage->Height; y++)
            {
#ifndef HAS_SDL            
              BYTE *dest = (BYTE *)lr.pBits + y * lr.Pitch;
#else
				      BYTE *dest = (BYTE *)pixels + (y * w * 4); 
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
#endif

#ifndef HAS_SDL
            pTexture->UnlockRect( 0 );
#else
				    SDL_UnlockSurface(pTexture);
#endif            

            CTexture* pclsTexture = new CTexture(pTexture, iWidth, iHeight, false, 100, pPal);
            pclsTexture->SetDelay(pImage->Delay);
            pclsTexture->SetLoops(AnimatedGifSet.nLoops);

#ifdef HAS_SDL_2D
            SDL_FreeSurface(pTexture);
#endif
            
            pMap->Add(pclsTexture);
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
    sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, bPacked ? " (packed)" : (bundle >= 0) ? " (bundled)" : "");
    OutputDebugString(temp);
#endif

    m_vecTextures.push_back(pMap);
    return pMap->size();
  } // of if (strPath.Right(4).ToLower()==".gif")

  if (bundle >= 0)
  {
#ifndef HAS_SDL
    if (FAILED(m_TexBundle[bundle].LoadTexture(g_graphicsContext.Get3DDevice(), *pstrBundleTex, &info, &pTexture, &pPal)))
#else    
    if (FAILED(m_TexBundle[bundle].LoadTexture(*pstrBundleTex, &info, &pTexture, &pPal)))
#endif    
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", pstrBundleTex->c_str());
      return 0;
    }
  }
  else if (bPacked)
  {
#ifdef HAS_XBOX_D3D
    if (FAILED(LoadPackedTexture(g_graphicsContext.Get3DDevice(), strPackedPath.c_str(), &info, &pTexture, &pPal)))
#endif
    {
      if (!strnicmp(strPackedPath.c_str(), "q:\\skin", 7))
        CLog::Log(LOGERROR, "Texture manager unable to load packed file: %s", strPackedPath.c_str());
      return 0;
    }
  }
  else
  {
    if (strPath.Right(4).ToLower() == ".dds")
    {
#ifndef HAS_SDL    
      if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
                                       D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                       D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
#endif                                       
      {
        if (!strnicmp(strPath.c_str(), "q:\\skin", 7))        
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return 0;
      }
    }
    else
    {    
      // normal picture
      // convert from utf8
      CStdString texturePath;
      g_charsetConverter.utf8ToStringCharset(_P(strPath), texturePath);
      
#ifndef HAS_SDL
      if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), texturePath.c_str(),
                                       D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
                                       D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
      {
        if (!strnicmp(strPath.c_str(), "q:\\skin", 7))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return 0;

      }
#else
	
      SDL_Surface *original = IMG_Load(texturePath.c_str());
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
      format.Amask = 0xff000000; format.Ashift = 24;
      format.Rmask = 0x00ff0000; format.Rshift = 16;
      format.Gmask = 0x0000ff00; format.Gshift = 8;
      format.Bmask = 0x000000ff; format.Bshift = 0;
      
      pTexture = SDL_ConvertSurface(original, &format, SDL_HWSURFACE);
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
  }

#ifdef _DEBUG
  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  char temp[200];
  sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, bPacked ? " (packed)" : (bundle >= 0) ? " (bundled)" : "");
  OutputDebugString(temp);
#endif

  CTextureMap* pMap = new CTextureMap(strTextureName);
  CTexture* pclsTexture = new CTexture(pTexture, info.Width, info.Height, bPacked || bundle >= 0, 100, pPal);
  pMap->Add(pclsTexture);
  m_vecTextures.push_back(pMap);

#ifdef HAS_SDL_OPENGL
  SDL_FreeSurface(pTexture);
#endif    
  
#ifdef HAS_XBOX_D3D
  if (pPal)
    pPal->Release();
#endif
#else

  LPDIRECT3DTEXTURE8 pTexture;
  CStdString strPath = GetTexturePath(strTextureName);

  //OutputDebugString(strPath.c_str());
  //OutputDebugString("\n");

  if (strPath.Right(4).ToLower() == ".gif")
  {

    CAnimatedGifSet AnimatedGifSet;
    int iImages = AnimatedGifSet.LoadGIF(strPath.c_str());
    if (iImages == 0)
    {
      CStdString strText = strPath;
      strText.MakeLower();
      // dont release skin textures, they are reloaded each time
      if (strstr(strPath.c_str(), "q:\\skin") )
      {
        CLog::Log(LOGERROR, "Texture manager unable to find file:%s", strPath.c_str());
      }
      return 0;
    }
    int iWidth = AnimatedGifSet.FrameWidth;
    int iHeight = AnimatedGifSet.FrameHeight;

    CTextureMap* pMap = new CTextureMap(strTextureName);
    for (int iImage = 0; iImage < iImages; iImage++)
    {
      if (D3DXCreateTexture(g_graphicsContext.Get3DDevice(), iWidth,
          iHeight,
          1,  // levels
          0,  //usage
          D3DFMT_LIN_A8R8G8B8 ,
          0,
          &pTexture) == D3D_OK)
      {
        CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];
        //dllprintf("%s loops:%i", strTextureName.c_str(),AnimatedGifSet.nLoops);
        D3DLOCKED_RECT lr;
        if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
        {
          DWORD strideScreen = lr.Pitch;
          for (DWORD y = 0; y < (DWORD)pImage->Height; y++)
          {
            BYTE *pDest = (BYTE*)lr.pBits + strideScreen * y;
            for (DWORD x = 0;x < (DWORD)pImage->Width; x++)
            {
              byte byAlpha = 0xff;
              byte iPaletteColor = (byte)pImage->Pixel( x, y);
              if (pImage->Transparency)
              {
                int iTransparentColor = pImage->Transparent;
                if (iTransparentColor < 0) iTransparentColor = 0;
                if (iPaletteColor == iTransparentColor)
                {
                  byAlpha = 0x0;
                }
              }
              COLOR& Color = pImage->Palette[iPaletteColor];

              *pDest++ = Color.b;
              *pDest++ = Color.g;
              *pDest++ = Color.r;
              *pDest++ = byAlpha;
            } // of for (DWORD x=0; x < (DWORD)pImage->Width; x++)
          } // of for (DWORD y=0; y < (DWORD)pImage->Height; y++)
          pTexture->UnlockRect( 0 );
        } // of if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
        CTexture* pclsTexture = new CTexture(pTexture, iWidth, iHeight, false);
        pclsTexture->SetDelay(pImage->Delay);
        pclsTexture->SetLoops(AnimatedGifSet.nLoops);

        pMap->Add(pclsTexture);
      } // of if (g_graphicsContext.Get3DDevice()->CreateTexture
    } // of for (int iImage=0; iImage < iImages; iImage++)
    m_vecTextures.push_back(pMap);
    return pMap->size();
  } // of if (strPath.Right(4).ToLower()==".gif")

  // normal picture
  D3DXIMAGE_INFO info;
  if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
                                   D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                   D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
  {
    CStdString strText = strPath;
    strText.MakeLower();
    // dont release skin textures, they are reloaded each time
    if (strstr(strPath.c_str(), "q:\\skin") )
    {
      CLog::Log(LOGERROR, "Texture manager unable to find file:%s", strPath.c_str());
    }
    return NULL;
  }
  //CStdString strLog;
  //strLog.Format("%s %ix%i\n", strTextureName.c_str(),info.Width,info.Height);
  //OutputDebugString(strLog.c_str());
  CTextureMap* pMap = new CTextureMap(strTextureName);
  CTexture* pclsTexture = new CTexture(pTexture, info.Width, info.Height, false);
  pMap->Add(pclsTexture);
  m_vecTextures.push_back(pMap);
  return 1;

#endif
  return 1;
}

void CGUITextureManager::ReleaseTexture(const CStdString& strTextureName, int iPicture)
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
      pMap->Release(iPicture);

      if (pMap->IsEmpty() )
      {
        //CLog::Log(LOGINFO, "  cleanup:%s", strTextureName.c_str());
        delete pMap;
        i = m_vecTextures.erase(i);
      }
      else
      {
        ++i;
      }
      //++i;
    }
    else
    {
      ++i;
    }
  }
}

void CGUITextureManager::Cleanup()
{
  CSingleLock lock(g_graphicsContext);

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap = *i;
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
    {
      strLog.Format("map:%i\n", i);
      OutputDebugString(strLog.c_str());
      pMap->Dump();
    }
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

CStdString CGUITextureManager::GetTexturePath(const CStdString &textureName)
{
  CStdString path;
#ifndef _LINUX  
  if (textureName.c_str()[1] == ':')
#else
  if (textureName.c_str()[0] == '/')
#endif  
    path = textureName;
  else
    path.Format("%s\\media\\%s", g_graphicsContext.GetMediaDir().c_str(), textureName.c_str());
  return _P(path);
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
 
  glTexImage2D(GL_TEXTURE_2D, 0, 4, textureWidth, textureHeight, 0,
               GL_BGRA, GL_UNSIGNED_BYTE, m_pixels);
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
    memcpy(resized, src, tpitch); // make sure pitch is not bigger than our width
    src += pitch;
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

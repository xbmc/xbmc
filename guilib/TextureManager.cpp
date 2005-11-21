#include "include.h"
#include "TextureManager.h"
#include "AnimatedGif.h"
#include "PackedTexture.h"
#include "GraphicContext.h"
#include <XGraphics.h>


extern void fast_memcpy(void* d, const void* s, unsigned n);

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
}

CTexture::CTexture(LPDIRECT3DTEXTURE8 pTexture, int iWidth, int iHeight, bool bPacked, int iDelay, LPDIRECT3DPALETTE8 pPalette)
{
  m_iLoops = 0;
  m_iReferenceCount = 0;
  m_pTexture = pTexture;
  m_iDelay = 2 * iDelay;
  m_iWidth = iWidth;
  m_iHeight = iHeight;
  m_pPalette = pPalette;
  if (m_pPalette)
    m_pPalette->AddRef();
  m_bPacked = bPacked;
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
      m_pTexture->BlockUntilNotBusy();
      void* Data = (void*)(*(DWORD*)(((char*)m_pTexture) + sizeof(D3DTexture)));
      if (Data)
        XPhysicalFree(Data);
      delete [] m_pTexture;
    }
    else
      m_pTexture->Release();
    m_pTexture = NULL;
  }
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


LPDIRECT3DTEXTURE8 CTexture::GetTexture(int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal)
{
  if (!m_pTexture) return NULL;
  m_iReferenceCount++;
  iWidth = m_iWidth;
  iHeight = m_iHeight;
  pPal = m_pPalette;
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


LPDIRECT3DTEXTURE8 CTextureMap::GetTexture(int iPicture, int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal)
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return NULL;

  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->GetTexture(iWidth, iHeight, pPal);
}

void CTextureMap::Flush()
{
  for (int i = 0; i < (int)m_vecTexures.size(); ++i)
  {
    m_vecTexures[i]->Flush();
  }
}

//------------------------------------------------------------------------------
CGUITextureManager::CGUITextureManager(void)
{
  D3DXSetDXT3DXT5(TRUE);
  m_iNextPreload = m_PreLoadNames.end();
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}


LPDIRECT3DTEXTURE8 CGUITextureManager::GetTexture(const CStdString& strTextureName, int iItem, int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal)
{
  //  CLog::Log(LOGINFO, " refcount++ for  GetTexture(%s)\n", strTextureName.c_str());
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return pMap->GetTexture(iItem, iWidth, iHeight, pPal);
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
  m_PreLoadNames.clear();
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

  for (list<CStdString>::iterator i = m_PreLoadNames.begin(); i != m_PreLoadNames.end(); ++i)
  {
    if (*i == strTextureName)
      return ;
  }

  if (m_TexBundle.HasFile(strTextureName))
    m_PreLoadNames.push_back(strTextureName);
}

void CGUITextureManager::EndPreLoad()
{
  m_iNextPreload = m_PreLoadNames.begin();
  // preload next file
  if (m_iNextPreload != m_PreLoadNames.end())
    m_TexBundle.PreloadFile(*m_iNextPreload);
}

void CGUITextureManager::FlushPreLoad()
{
  m_PreLoadNames.clear();
  m_iNextPreload = m_PreLoadNames.end();
}

int CGUITextureManager::Load(const CStdString& strTextureName, DWORD dwColorKey)
{
  if (strTextureName == "-")
    return NULL;

  // first check of texture exists...
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap = m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      if (m_iNextPreload != m_PreLoadNames.end() && (*m_iNextPreload == strTextureName))
      {
        ++m_iNextPreload;
        // preload next file
        if (m_iNextPreload != m_PreLoadNames.end())
          m_TexBundle.PreloadFile(*m_iNextPreload);
      }

      return pMap->size();
    }
  }

  //Lock here, we will do stuff that could break rendering
  CSingleLock lock(g_graphicsContext);

#ifdef ALLOW_TEXTURE_COMPRESSION
  LPDIRECT3DTEXTURE8 pTexture;
  LPDIRECT3DPALETTE8 pPal = 0;

  bool bBundled;
  const CStdString* pstrBundleTex = NULL;
  if (m_iNextPreload != m_PreLoadNames.end() && (*m_iNextPreload == strTextureName)) // || *m_iNextPreload == strPalTex))
  {
    pstrBundleTex = &strTextureName;

    bBundled = true;
    ++m_iNextPreload;
    // preload next file
    if (m_iNextPreload != m_PreLoadNames.end())
      m_TexBundle.PreloadFile(*m_iNextPreload);
  }
  else if (m_TexBundle.HasFile(strTextureName))
  {
    pstrBundleTex = &strTextureName;
    bBundled = true;
  }
  else
    bBundled = false;

  CStdString strPath;

  if (!bBundled)
    strPath = GetTexturePath(strTextureName);
  else
    strPath = strTextureName;

  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);

  bool bPacked;
  CStdString strPackedPath;
  if (!bBundled)
  {
    strPackedPath = strPath;
    strPackedPath += ".xpr";
    bPacked = (GetFileAttributes(strPackedPath.c_str()) != -1);
  }
  else
    bPacked = false;

  D3DXIMAGE_INFO info;

  if (strPath.Right(4).ToLower() == ".gif")
  {
    CTextureMap* pMap;

    if (bPacked || bBundled)
    {
      LPDIRECT3DTEXTURE8* pTextures;
      int nLoops = 0;
      int* Delay;
      int nImages;
      if (bBundled)
      {
        nImages = m_TexBundle.LoadAnim(g_graphicsContext.Get3DDevice(), *pstrBundleTex, &info, &pTextures, &pPal, nLoops, &Delay);
        if (!nImages)
        {
          CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", pstrBundleTex->c_str());
          return NULL;
        }
      }
      else // packed
      {
        nImages = LoadPackedAnim(g_graphicsContext.Get3DDevice(), strPackedPath.c_str(), &info, &pTextures, &pPal, nLoops, &Delay);
        if (!nImages)
        {
          if (!strnicmp(strPackedPath.c_str(), "q:\\skin", 7))
            CLog::Log(LOGERROR, "Texture manager unable to load packed file: %s", strPackedPath.c_str());
          return NULL;
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
      pPal->Release();
    }
    else
    {
      CAnimatedGifSet AnimatedGifSet;
      int iImages = AnimatedGifSet.LoadGIF(strPath.c_str());
      if (iImages == 0)
      {
        if (!strnicmp(strPath.c_str(), "q:\\skin", 7))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return NULL;
      }
      int iWidth = AnimatedGifSet.FrameWidth;
      int iHeight = AnimatedGifSet.FrameHeight;

      int iPalletSize = (1 << AnimatedGifSet.m_vecimg[0]->BPP);
      g_graphicsContext.Get3DDevice()->CreatePalette(D3DPALETTE_256, &pPal);
      PALETTEENTRY* pal;
      pPal->Lock((D3DCOLOR**)&pal, 0);

      fast_memcpy(pal, AnimatedGifSet.m_vecimg[0]->Palette, sizeof(PALETTEENTRY)*iPalletSize);
      for (int i = 0; i < iPalletSize; i++)
        pal[i].peFlags = 0xff; // alpha
      if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
        pal[AnimatedGifSet.m_vecimg[0]->Transparent].peFlags = 0;

      pPal->Unlock();

      pMap = new CTextureMap(strTextureName);
      for (int iImage = 0; iImage < iImages; iImage++)
      {
        int w = PadPow2(iWidth);
        int h = PadPow2(iHeight);
        if (g_graphicsContext.Get3DDevice()->CreateTexture(w, h, 1, 0, D3DFMT_P8, 0, &pTexture) == D3D_OK)
        {
          D3DLOCKED_RECT lr;
          CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];
          RECT rc = { 0, 0, pImage->Width, pImage->Height };
          if ( D3D_OK == pTexture->LockRect( 0, &lr, &rc, 0 ))
          {
            POINT pt = { 0, 0 };
            XGSwizzleRect(pImage->Raster, pImage->BytesPerRow, &rc, lr.pBits, w, h, &pt, 1);

            pTexture->UnlockRect( 0 );

            CTexture* pclsTexture = new CTexture(pTexture, iWidth, iHeight, false, 100, pPal);
            pclsTexture->SetDelay(pImage->Delay);
            pclsTexture->SetLoops(AnimatedGifSet.nLoops);

            pMap->Add(pclsTexture);
          }
        }
      } // of for (int iImage=0; iImage < iImages; iImage++)

      pPal->Release();
    }

    LARGE_INTEGER end, freq;
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);
    char temp[200];
    sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, bPacked ? " (packed)" : bBundled ? " (bundled)" : "");
    OutputDebugString(temp);

    m_vecTextures.push_back(pMap);
    return pMap->size();
  } // of if (strPath.Right(4).ToLower()==".gif")

  if (bBundled)
  {
    if (FAILED(m_TexBundle.LoadTexture(g_graphicsContext.Get3DDevice(), *pstrBundleTex, &info, &pTexture, &pPal)))
    {
      CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", pstrBundleTex->c_str());
      return NULL;
    }
  }
  else if (bPacked)
  {
    if (FAILED(LoadPackedTexture(g_graphicsContext.Get3DDevice(), strPackedPath.c_str(), &info, &pTexture, &pPal)))
    {
      if (!strnicmp(strPackedPath.c_str(), "q:\\skin", 7))
        CLog::Log(LOGERROR, "Texture manager unable to load packed file: %s", strPackedPath.c_str());
      return NULL;
    }
  }
  else
  {
    if (strPath.Right(4).ToLower() == ".dds")
    {
      if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
                                       D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                       D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
      {
        if (!strnicmp(strPath.c_str(), "q:\\skin", 7))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return NULL;
      }
    }
    else
    {
      // normal picture
      if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
                                       D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                       D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture) != D3D_OK)
      {
        if (!strnicmp(strPath.c_str(), "q:\\skin", 7))
          CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
        return NULL;
      }
    }
  }

  LARGE_INTEGER end, freq;
  QueryPerformanceCounter(&end);
  QueryPerformanceFrequency(&freq);
  // char temp[200];
  // sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, bPacked ? " (packed)" : bBundled ? " (bundled)" : "");
  // OutputDebugString(temp);

  //CStdString strLog;
  //strLog.Format("%s %ix%i\n", strTextureName.c_str(),info.Width,info.Height);
  //OutputDebugString(strLog.c_str());
  CTextureMap* pMap = new CTextureMap(strTextureName);
  CTexture* pclsTexture = new CTexture(pTexture, info.Width, info.Height, bPacked || bBundled, 100, pPal);
  pMap->Add(pclsTexture);
  m_vecTextures.push_back(pMap);
  if (pPal)
    pPal->Release();

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
      if (g_graphicsContext.Get3DDevice()->CreateTexture( iWidth,
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
                                   D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
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
  m_TexBundle.Cleanup();
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

CStdString CGUITextureManager::GetTexturePath(const CStdString &textureName)
{
  CStdString path;
  if (textureName.c_str()[1] == ':')
    path = textureName;
  else
    path.Format("%s\\media\\%s", g_graphicsContext.GetMediaDir().c_str(), textureName.c_str());
  return path;
}

void CGUITextureManager::GetBundledTexturesFromPath(const CStdString& texturePath, CStdStringArray &items)
{
  m_TexBundle.GetTexturesFromPath(texturePath, items);
}

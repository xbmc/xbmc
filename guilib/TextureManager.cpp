#include "texturemanager.h"
#include "graphiccontext.h"
#include "animatedgif.h"
#include "../xbmc/utils/log.h"
#include <xgraphics.h>

extern "C" void dllprintf( const char *format, ... );

CGUITextureManager g_TextureManager;

CTexture::CTexture()
{
  m_iReferenceCount=0;
  m_pTexture=NULL;
  m_iDelay=100;
	m_iWidth=m_iHeight=0;
  m_iLoops=0;
	m_pPalette = NULL;
	m_bCached = false;
}

CTexture::CTexture(LPDIRECT3DTEXTURE8 pTexture,int iWidth, int iHeight, bool Cached, int iDelay, LPDIRECT3DPALETTE8 pPalette)
{
  m_iLoops=0;
  m_iReferenceCount=0;
  m_pTexture=pTexture;
  m_iDelay=iDelay;
	m_iWidth=iWidth;
	m_iHeight=iHeight;
	m_pPalette = pPalette;
	if (m_pPalette)
		m_pPalette->AddRef();
	m_bCached = Cached;
}

CTexture::~CTexture()
{
	if (m_pPalette)
		m_pPalette->Release();
	m_pPalette=NULL;

	FreeTexture();
}

void CTexture::FreeTexture()
{
	if (m_pTexture)
	{
		if (m_bCached)
		{
			m_pTexture->BlockUntilNotBusy();
			void* Data = (void*)(*(DWORD*)(((char*)m_pTexture) + sizeof(D3DTexture)));
			D3D_FreeContiguousMemory(Data);
			delete [] (char*)m_pTexture;
		}
		else
			m_pTexture->Release();
		m_pTexture=NULL;
	}
}

void CTexture::Dump() const
{
  if (!m_iReferenceCount) return;
  CStdString strLog;
  strLog.Format("refcount:%i\n:", m_iReferenceCount);
  OutputDebugString(strLog.c_str());
}

void CTexture::SetLoops(int iLoops)
{
  m_iLoops=iLoops;
}

int CTexture::GetLoops() const
{
  return m_iLoops;
}

void CTexture::SetDelay(int iDelay)
{
  if (iDelay)
  {
    m_iDelay=2*iDelay;
  }
  else
  {
    m_iDelay=100;
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
  if (!m_iReferenceCount)  return true;
  if (m_iReferenceCount>0)
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
	iWidth=m_iWidth;
	iHeight=m_iHeight;
	pPal = m_pPalette;
  return m_pTexture;
}

//-----------------------------------------------------------------------------
CTextureMap::CTextureMap()
{
  m_strTextureName="";
}

CTextureMap::CTextureMap(const CStdString& strTextureName)
{
   m_strTextureName=strTextureName;
}

CTextureMap::~CTextureMap()
{
  for (int i=0; i < (int)m_vecTexures.size(); ++i)
  {
    CTexture* pTexture = m_vecTexures[i];
    delete pTexture ;
  }
  m_vecTexures.erase(m_vecTexures.begin(),m_vecTexures.end());
}

void CTextureMap::Dump() const
{
  if (IsEmpty()) return;
  CStdString strLog;
  strLog.Format("  texure:%s has %i frames\n", m_strTextureName.c_str(), m_vecTexures.size());
  OutputDebugString(strLog.c_str());

  for (int i=0; i < (int)m_vecTexures.size(); ++i)
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
  return  m_vecTexures.size();
}

bool CTextureMap::IsEmpty() const
{
  int iRef=0;
  for (int i=0; i < (int)m_vecTexures.size(); ++i)
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


LPDIRECT3DTEXTURE8 CTextureMap::GetTexture(int iPicture,int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal)
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return NULL;
  
  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->GetTexture(iWidth,iHeight,pPal);
}

void CTextureMap::Flush()
{
	for (int i=0; i < (int)m_vecTexures.size(); ++i)
	{
		m_vecTexures[i]->Flush();
	}
}

//------------------------------------------------------------------------------
CGUITextureManager::CGUITextureManager(void)
{
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}


LPDIRECT3DTEXTURE8 CGUITextureManager::GetTexture(const CStdString& strTextureName,int iItem, int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal)
{
//  CLog::Log(" refcount++ for  GetTexture(%s)\n", strTextureName.c_str());
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap=m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return  pMap->GetTexture(iItem, iWidth,iHeight,pPal);
    }
  }
  return NULL;
}

int CGUITextureManager::GetLoops(const CStdString& strTextureName, int iPicture) const
{
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap=m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return  pMap->GetLoops(iPicture);
    }
  }
  return 0;
}
int CGUITextureManager::GetDelay(const CStdString& strTextureName, int iPicture) const
{
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap=m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return  pMap->GetDelay(iPicture);
    }
  }
  return 100;
}

int RoundPow2(int s)
{
	float f = log((float)s)/log(2.f);
	return 1 << (int)(f+0.999f);
}

static HRESULT LoadCachedTexture(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture)
{
	*ppTexture = NULL;

	HANDLE hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	XPR_HEADER XPRHeader;
	DWORD n;
	if (!ReadFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, NULL) || n < sizeof(XPR_HEADER))
	{
		CloseHandle(hFile);
		return E_INVALIDARG;
	}

	if (XPRHeader.dwMagic != XPR_MAGIC_VALUE)
	{
		CloseHandle(hFile);
		return E_INVALIDARG;
	}

	DWORD ResHeaderSize = XPRHeader.dwHeaderSize - sizeof(XPR_HEADER);
	char* ResHeader = new char[sizeof(D3DTexture) + sizeof(DWORD)];
	if (!ResHeader)
	{
		CloseHandle(hFile);
		return E_FAIL;
	}
	DWORD ResDataSize = XPRHeader.dwTotalSize - XPRHeader.dwHeaderSize;
	void* ResData = D3D_AllocContiguousMemory(ResDataSize, 4096);
	if (!ResData)
	{
		delete [] ResHeader;
		CloseHandle(hFile);
		return E_FAIL;
	}

	if (!ReadFile(hFile, ResHeader, sizeof(D3DTexture) + sizeof(DWORD), &n, NULL) || n != sizeof(D3DTexture) + sizeof(DWORD))
	{
		delete [] ResHeader;
		D3D_FreeContiguousMemory(ResData);
		CloseHandle(hFile);
		return E_FAIL;
	}
	SetFilePointer(hFile, ResHeaderSize - (sizeof(D3DTexture) + sizeof(DWORD)), NULL, FILE_CURRENT);
	if (!ReadFile(hFile, ResData, ResDataSize, &n, NULL) || n != ResDataSize)
	{
		delete [] ResHeader;
		D3D_FreeContiguousMemory(ResData);
		CloseHandle(hFile);
		return E_FAIL;
	}
	WORD RealSize[2];
	if (!ReadFile(hFile, RealSize, 4, &n, 0) || n != 4)
	{
		delete [] ResHeader;
		D3D_FreeContiguousMemory(ResData);
		CloseHandle(hFile);
		return E_FAIL;
	}
	
	CloseHandle(hFile);

	if ((*((DWORD *)ResHeader) & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
	{
		delete [] ResHeader;
		D3D_FreeContiguousMemory(ResData);
		return E_INVALIDARG;
	}

	*ppTexture = (LPDIRECT3DTEXTURE8)ResHeader;
	(*ppTexture)->Register(ResData);
	*(DWORD*)(ResHeader + sizeof(D3DTexture)) = (DWORD)ResData;

	pInfo->Width = RealSize[0];
	pInfo->Height = RealSize[1];
	pInfo->Depth = 0;
	pInfo->MipLevels = 1;
	D3DSURFACE_DESC desc;
	(*ppTexture)->GetLevelDesc(0, &desc);
	pInfo->Format = desc.Format;

	return S_OK;
}

int CGUITextureManager::Load(const CStdString& strTextureName,DWORD dwColorKey)
{
  // first check of texture exists...
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap=m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return pMap->size();
    }
  }
#if ALLOW_TEXTURE_COMPRESSION
  LPDIRECT3DTEXTURE8 pTexture;
  CStdString strPath=g_graphicsContext.GetMediaDir();
	strPath+="\\media\\";
  strPath+=strTextureName;
  if (strTextureName.c_str()[1] == ':')
    strPath=strTextureName;

  //OutputDebugString(strPath.c_str());
  //OutputDebugString("\n");

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);

  if (strPath.Right(4).ToLower()==".gif")
  {
 
  	CAnimatedGifSet		AnimatedGifSet;
    int iImages=AnimatedGifSet.LoadGIF(strPath.c_str());
		if (iImages==0)
		{
      CStdString strText=strPath;
      strText.MakeLower();
      // dont release skin textures, they are reloaded each time
      if (strstr(strPath.c_str(),"q:\\skin") ) 
      {
        CLog::Log("Texture manager unable to find file:%s",strPath.c_str());
      }
			return 0;
		}
    int iWidth = AnimatedGifSet.FrameWidth;
		int iHeight= AnimatedGifSet.FrameHeight;

		IDirect3DPalette8* pPal;
		g_graphicsContext.Get3DDevice()->CreatePalette(D3DPALETTE_256, &pPal);
		PALETTEENTRY* pal;
		pPal->Lock((D3DCOLOR**)&pal, 0);

		memcpy(pal, AnimatedGifSet.m_vecimg[0]->Palette, sizeof(PALETTEENTRY)*256);
		for (int i = 0; i < 256; i++)
			pal[i].peFlags = 0xff; // alpha
		if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
			pal[AnimatedGifSet.m_vecimg[0]->Transparent].peFlags = 0;

		pPal->Unlock();

		CTextureMap* pMap = new CTextureMap(strTextureName);
	  for (int iImage=0; iImage < iImages; iImage++)
    {
			int w = RoundPow2(iWidth);
			int h = RoundPow2(iHeight);
			if (g_graphicsContext.Get3DDevice()->CreateTexture(w, h, 1, 0, D3DFMT_P8, 0, &pTexture) == D3D_OK) 
			{
				D3DLOCKED_RECT lr;
				CAnimatedGif* pImage=AnimatedGifSet.m_vecimg[iImage];
				RECT rc = { 0, 0, pImage->Width, pImage->Height };
				if ( D3D_OK == pTexture->LockRect( 0, &lr, &rc, 0 ))
				{
					POINT pt = { 0, 0 };
					XGSwizzleRect(pImage->Raster, pImage->BytesPerRow, &rc, lr.pBits, w, h, &pt, 1);

					pTexture->UnlockRect( 0 );

					CTexture* pclsTexture = new CTexture(pTexture,iWidth,iHeight, false, 100, pPal);
					pclsTexture->SetDelay(pImage->Delay);
					pclsTexture->SetLoops(AnimatedGifSet.nLoops);
					
					pMap->Add(pclsTexture);
				}
			}

    } // of for (int iImage=0; iImage < iImages; iImage++)

		pPal->Release();

		LARGE_INTEGER end, freq;
		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);
		char temp[200];
		sprintf(temp, "Load %s: %.1fms\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart);
		OutputDebugString(temp);

    m_vecTextures.push_back(pMap);
    return pMap->size();
  } // of if (strPath.Right(4).ToLower()==".gif")

	D3DXSetDXT3DXT5(TRUE);

	bool Cached = false;
	CStdString strCachePath(strPath);
	int n = strCachePath.find_last_of('\\');
	strCachePath.insert(n, "\\cache");
	strCachePath += ".xpr";
	if (GetFileAttributes(strCachePath) != -1)
	{
		FILETIME t1, t2;
		WIN32_FIND_DATA FindData;
		HANDLE hFind = FindFirstFile(strPath, &FindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);
			memcpy(&t1, &FindData.ftLastWriteTime, sizeof(FILETIME));

			HANDLE hFind = FindFirstFile(strCachePath, &FindData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				FindClose(hFind);
				memcpy(&t2, &FindData.ftLastWriteTime, sizeof(FILETIME));

				if (!CompareFileTime(&t1, &t2))
				{
					Cached = true;
				}
			}
		}
	}

	D3DXIMAGE_INFO info;

	if (Cached)
	{
		if (FAILED(LoadCachedTexture(g_graphicsContext.Get3DDevice(), strCachePath.c_str(), &info, &pTexture)))
		{
			CStdString strText = strCachePath;
			strText.MakeLower();
			// dont release skin textures, they are reloaded each time
			if (strstr(strText.c_str(),"q:\\skin") ) 
			{
				CLog::Log("Texture manager unable to load file:%s",strCachePath.c_str());
			}
			return NULL;
		}
	}
	else
	{
		if (strPath.Right(4).ToLower() == ".dds")
		{
			if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
				D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
				D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture)!=D3D_OK)
			{
				CStdString strText = strPath;
				strText.MakeLower();
				// dont release skin textures, they are reloaded each time
				if (strstr(strText.c_str(),"q:\\skin") ) 
				{
					CLog::Log("Texture manager unable to load file:%s",strPath.c_str());
				}
				return NULL;
			}
		}
		else
		{
			// normal picture
			if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
				D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_DXT3, D3DPOOL_MANAGED,
				D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture)!=D3D_OK)
			{
				CStdString strText = strPath;
				strText.MakeLower();
				// dont release skin textures, they are reloaded each time
				if (strstr(strText.c_str(),"q:\\skin") ) 
				{
					CLog::Log("Texture manager unable to load file:%s",strPath.c_str());
				}
				return NULL;
			}
		}

		// save cached copy (if skin texture)
		CStdString strText = strCachePath;
		strText.MakeLower();
		if (strstr(strText.c_str(),"q:\\skin") ) 
		{
			WIN32_FIND_DATA FindData;
			HANDLE hFind = FindFirstFile(strPath, &FindData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				FindClose(hFind);

				XGWriteSurfaceOrTextureToXPR(pTexture, strCachePath.c_str(), true);

				HANDLE hFile = CreateFile(strCachePath.c_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					SetFilePointer(hFile, 0, NULL, FILE_END);
					WORD Size[2];
					Size[0] = info.Width;
					Size[1] = info.Height;
					DWORD n;
					WriteFile(hFile, Size, 4, &n, 0);
					SetFileTime(hFile, NULL, NULL, &FindData.ftLastWriteTime);
					CloseHandle(hFile);
				}
			}
		}
	}

	LARGE_INTEGER end, freq;
	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&freq);
	char temp[200];
	sprintf(temp, "Load %s: %.1fms %s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, Cached ? "(cached)" : "");
	OutputDebugString(temp);

	//CStdString strLog;
	//strLog.Format("%s %ix%i\n", strTextureName.c_str(),info.Width,info.Height);
	//OutputDebugString(strLog.c_str());
  CTextureMap* pMap = new CTextureMap(strTextureName);
	CTexture* pclsTexture = new CTexture(pTexture,info.Width,info.Height, Cached);
  pMap->Add(pclsTexture);
  m_vecTextures.push_back(pMap);
#else
  LPDIRECT3DTEXTURE8 pTexture;
  CStdString strPath=g_graphicsContext.GetMediaDir();
	strPath+="\\media\\";
  strPath+=strTextureName;
  if (strTextureName.c_str()[1] == ':')
    strPath=strTextureName;

  //OutputDebugString(strPath.c_str());
  //OutputDebugString("\n");

  if (strPath.Right(4).ToLower()==".gif")
  {
 
  	CAnimatedGifSet		AnimatedGifSet;
    int iImages=AnimatedGifSet.LoadGIF(strPath.c_str());
		if (iImages==0)
		{
      CStdString strText=strPath;
      strText.MakeLower();
      // dont release skin textures, they are reloaded each time
      if (strstr(strPath.c_str(),"q:\\skin") ) 
      {
        CLog::Log("Texture manager unable to find file:%s",strPath.c_str());
      }
			return 0;
		}
    int iWidth = AnimatedGifSet.FrameWidth;
		int iHeight= AnimatedGifSet.FrameHeight;

    CTextureMap* pMap = new CTextureMap(strTextureName);
	  for (int iImage=0; iImage < iImages; iImage++)
    {
      if (g_graphicsContext.Get3DDevice()->CreateTexture( iWidth, 
																												  iHeight, 
																												  1, // levels
																												  0, //usage
																												  D3DFMT_LIN_A8R8G8B8 ,
																												  0,
																												  &pTexture) == D3D_OK) 
		  {
        CAnimatedGif* pImage=AnimatedGifSet.m_vecimg[iImage];
        //dllprintf("%s loops:%i", strTextureName.c_str(),AnimatedGifSet.nLoops);
        D3DLOCKED_RECT lr;
	      if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
	      {
		      DWORD strideScreen=lr.Pitch;
		      for (DWORD y=0; y <(DWORD)pImage->Height; y++)
		      {
			      BYTE *pDest = (BYTE*)lr.pBits + strideScreen*y;
			      for (DWORD x=0;x <(DWORD)pImage->Width; x++)
			      {
				      byte byAlpha=0xff;
				      byte iPaletteColor=(byte)pImage->Pixel( x, y);
              if (pImage->Transparency)
              {
                int iTransparentColor=pImage->Transparent;
                if (iTransparentColor<0) iTransparentColor=0;
				        if (iPaletteColor==iTransparentColor)
				        {
					        byAlpha=0x0;
				        }
              }
				      COLOR& Color= pImage->Palette[iPaletteColor];
      				
				      *pDest++ = Color.b;
				      *pDest++ = Color.g;
				      *pDest++ = Color.r;
              *pDest++ = byAlpha;
			      } // of for (DWORD x=0; x < (DWORD)pImage->Width; x++)
		      } // of for (DWORD y=0; y < (DWORD)pImage->Height; y++)
		      pTexture->UnlockRect( 0 );
	      } // of if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
        CTexture* pclsTexture = new CTexture(pTexture,iWidth,iHeight,false);
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
		 D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture)!=D3D_OK)
	{
      CStdString strText=strPath;
      strText.MakeLower();
      // dont release skin textures, they are reloaded each time
      if (strstr(strPath.c_str(),"q:\\skin") ) 
      {
        CLog::Log("Texture manager unable to find file:%s",strPath.c_str());
      }
		return NULL;
	}
	//CStdString strLog;
	//strLog.Format("%s %ix%i\n", strTextureName.c_str(),info.Width,info.Height);
	//OutputDebugString(strLog.c_str());
  CTextureMap* pMap = new CTextureMap(strTextureName);
	CTexture* pclsTexture = new CTexture(pTexture,info.Width,info.Height,false);
  pMap->Add(pclsTexture);
  m_vecTextures.push_back(pMap);
  return 1;

#endif
  return 1;
}

void CGUITextureManager::ReleaseTexture(const CStdString& strTextureName, int iPicture)
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus(&stat);
	DWORD dwMegFree=stat.dwAvailPhys / (1024*1024);
  if (dwMegFree>29)
  {
    // dont release skin textures, they are reloaded each time
    //if (strTextureName.GetAt(1) != ':') return;
    //CLog::Log("release:%s", strTextureName.c_str());
  }

  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap=*i;
    if (pMap->GetName()==strTextureName)
    {
      pMap->Release(iPicture);

      if (pMap->IsEmpty() )
      {
        //CLog::Log("  cleanup:%s", strTextureName.c_str());
        delete pMap;
        i=m_vecTextures.erase(i);
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
  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTextureMap* pMap=*i;
    delete pMap;
    i=m_vecTextures.erase(i);
  }
}

void CGUITextureManager::Dump() const
{
  CStdString strLog;
  strLog.Format("total texturemaps size:%i\n", m_vecTextures.size());
  OutputDebugString(strLog.c_str());

  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    const CTextureMap* pMap= m_vecTextures[i];
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
	ivecTextures i;
	i = m_vecTextures.begin();
	while (i != m_vecTextures.end())
	{
		CTextureMap* pMap=*i;
		pMap->Flush();
		if (pMap->IsEmpty() )
		{
			delete pMap;
			i=m_vecTextures.erase(i);
		}
		else 
		{
			++i;
		}
	}
}
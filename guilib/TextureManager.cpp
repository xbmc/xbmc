#include "texturemanager.h"
#include "graphiccontext.h"
#include "animatedgif.h"
#include "../xbmc/utils/log.h"

extern "C" void dllprintf( const char *format, ... );

CGUITextureManager g_TextureManager;

CTexture::CTexture()
{
  m_iReferenceCount=0;
  m_pTexture=NULL;
  m_iDelay=100;
	m_iWidth=m_iHeight=0;
  m_iLoops=0;
}

CTexture::CTexture(LPDIRECT3DTEXTURE8 pTexture,int iWidth, int iHeight, int iDelay)
{
  m_iLoops=0;
  m_iReferenceCount=0;
  m_pTexture=pTexture;
  m_iDelay=iDelay;
	m_iWidth=iWidth;
	m_iHeight=iHeight;
}

CTexture::~CTexture()
{
  if (m_pTexture)
    m_pTexture->Release();
  m_pTexture=NULL;
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
	{
	  m_pTexture->Release();
	  m_pTexture=NULL;
	}
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
    m_pTexture->Release();
    m_pTexture=NULL;
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


LPDIRECT3DTEXTURE8 CTexture::GetTexture(int& iWidth, int& iHeight)
{
  if (!m_pTexture) return NULL;
  m_iReferenceCount++;
	iWidth=m_iWidth;
	iHeight=m_iHeight;
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


LPDIRECT3DTEXTURE8 CTextureMap::GetTexture(int iPicture,int& iWidth, int& iHeight)
{
  if (iPicture < 0 || iPicture >= (int)m_vecTexures.size()) return NULL;
  
  CTexture* pTexture = m_vecTexures[iPicture];
  return pTexture->GetTexture(iWidth,iHeight);
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


LPDIRECT3DTEXTURE8 CGUITextureManager::GetTexture(const CStdString& strTextureName,int iItem, int& iWidth, int& iHeight)
{
//  CLog::Log(" refcount++ for  GetTexture(%s)\n", strTextureName.c_str());
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    CTextureMap *pMap=m_vecTextures[i];
    if (pMap->GetName() == strTextureName)
    {
      return  pMap->GetTexture(iItem, iWidth,iHeight);
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
																												  D3DFMT_DXT3 ,
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
        CTexture* pclsTexture = new CTexture(pTexture,iWidth,iHeight);
        pclsTexture->SetDelay(pImage->Delay);
        pclsTexture->SetLoops(AnimatedGifSet.nLoops);

        pMap->Add(pclsTexture);
		  } // of if (g_graphicsContext.Get3DDevice()->CreateTexture
    } // of for (int iImage=0; iImage < iImages; iImage++)
    m_vecTextures.push_back(pMap);
    return pMap->size();
  } // of if (strPath.Right(4).ToLower()==".gif")

	D3DXSetDXT3DXT5(TRUE);

	D3DXIMAGE_INFO info;

	int n = strPath.find_last_of('.');
	if (strPath.Right(strPath.size()-n) == ".dds")
	{
		if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
			D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
			D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, &info, NULL, &pTexture)!=D3D_OK)
		{
			CStdString strText = strPath;
			strText.MakeLower();
			// dont release skin textures, they are reloaded each time
			if (strstr(strPath.c_str(),"q:\\skin") ) 
			{
				CLog::Log("Texture manager unable to find file:%s",strPath.c_str());
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
			if (strstr(strPath.c_str(),"q:\\skin") ) 
			{
				CLog::Log("Texture manager unable to find file:%s",strPath.c_str());
			}
			return NULL;
		}
	}

	//CStdString strLog;
	//strLog.Format("%s %ix%i\n", strTextureName.c_str(),info.Width,info.Height);
	//OutputDebugString(strLog.c_str());
  CTextureMap* pMap = new CTextureMap(strTextureName);
	CTexture* pclsTexture = new CTexture(pTexture,info.Width,info.Height);
  pMap->Add(pclsTexture);
  m_vecTextures.push_back(pMap);
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
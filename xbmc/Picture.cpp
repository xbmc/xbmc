
#include "stdafx.h"
#include "picture.h"
#include "util.h"
#include "sectionloader.h"
#include "filesystem/file.h"
#include "utils/log.h"
#include "autoptrhandle.h"

using namespace AUTOPTR;

#define QUALITY 0
#define MAX_THUMB_WIDTH 128
#define MAX_THUMB_HEIGHT 128

#define MAX_ALBUM_THUMB_WIDTH 128
#define MAX_ALBUM_THUMB_HEIGHT 128

CPicture::CPicture(void)
{
	m_bSectionLoaded=false;
}

CPicture::~CPicture(void)
{
	Free();
}

void CPicture::Free()
{

	if ( m_bSectionLoaded)
	{
		CSectionLoader::Unload("CXIMAGE");
		m_bSectionLoaded=false;
	}
}

IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int iRotate,int iMaxWidth, int iMaxHeight, bool bRGB)
{
	IDirect3DTexture8* pTexture=NULL;
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType=0xffff;
	
	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return NULL;

	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tbn") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


  if (! CUtil::IsHD(strFileName))
  {
	  strCachedFile="T:\\cachedpic";
	  strCachedFile+= strExtension;
	  CFile file;
    ::DeleteFile(strCachedFile.c_str());
	  if ( !file.Cache(strFileName.c_str(),strCachedFile.c_str(),NULL,NULL) )
	  {
      ::DeleteFile(strCachedFile.c_str());
			CLog::Log("PICTURE::load: Unable to cache file %s\n", strFileName.c_str());
		  return NULL;
	  }
  }
  else
  {
    strCachedFile=strFileName;
  }

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

	CxImage image(dwImageType);
  try
  {
	  if (!image.Load(strCachedFile.c_str(),dwImageType))
	  {
			CLog::Log("PICTURE::load: Unable to open image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
		  return NULL;
	  }
  }
  catch(...)
  {
		CLog::Log("PICTURE::load: Unable to open image: %s\n", strCachedFile.c_str());
    if (CUtil::IsHD(strCachedFile) )
    {
      ::DeleteFile(strCachedFile.c_str());
    }
    return NULL;
  }

  for (int i=0; i < iRotate; ++i) 
  {
		image.RotateRight();
  }

	m_dwWidth  = image.GetWidth();
	m_dwHeight = image.GetHeight();
	
	bool bResize=false;
	float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);
		
	if (m_dwWidth > (DWORD)iMaxWidth)
    {
		bResize=true;
		m_dwWidth  = iMaxWidth;
		m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)iMaxHeight)
    {
		bResize=true;
		m_dwHeight = iMaxHeight;
		m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
		image.Resample(m_dwWidth,m_dwHeight, QUALITY);
	}

	m_dwWidth=image.GetWidth();
	m_dwHeight=image.GetHeight();

	image.Flip();
	if (bRGB)
		pTexture=GetTexture(image);
	else
		pTexture=GetYUY2Texture(image);
	return pTexture;
}

DWORD	CPicture::GetWidth()  const
{
	return m_dwWidth;
}

DWORD	CPicture::GetHeight()  const
{
	return m_dwHeight;
}


IDirect3DTexture8* CPicture::GetTexture( CxImage& image  )
{
	IDirect3DTexture8* pTexture=NULL;
	if (g_graphicsContext.Get3DDevice()->CreateTexture( image.GetWidth(), 
																											image.GetHeight(), 
																											1, // levels
																											0, //usage
																											D3DFMT_LIN_A8R8G8B8 ,
																											0,
																											&pTexture) != D3D_OK) 
	{
		return NULL;
	}
	if (!pTexture) 
	{
		return NULL;
	}

	DWORD dwHeight=image.GetHeight();
	DWORD dwWidth =image.GetWidth();

	D3DLOCKED_RECT lr;
	if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
	{
		DWORD strideScreen=lr.Pitch;
		for (DWORD y=0; y < dwHeight; y++)
		{
			BYTE *pDest = (BYTE*)lr.pBits + strideScreen*y;
			for (DWORD x=0; x < dwWidth; x++)
			{
				RGBQUAD rgb=image.GetPixelColor( x, y);
				
				*pDest++ = rgb.rgbBlue;
				*pDest++ = rgb.rgbGreen;
				*pDest++ = rgb.rgbRed;
        *pDest++ = 0xff;
			}
		}
		pTexture->UnlockRect( 0 );
	}
	return pTexture;
}

IDirect3DTexture8* CPicture::GetYUY2Texture( CxImage& image  )
{
	IDirect3DTexture8* pTexture=NULL;
	if (g_graphicsContext.Get3DDevice()->CreateTexture( image.GetWidth(), 
																											image.GetHeight(), 
																											1, // levels
																											0, //usage
																											D3DFMT_YUY2 ,
																											0,
																											&pTexture) != D3D_OK) 
	{
		return NULL;
	}
	if (!pTexture) 
	{
		return NULL;
	}

	DWORD dwHeight=image.GetHeight();
	DWORD dwWidth =image.GetWidth();

	D3DLOCKED_RECT lr;
	if ( D3D_OK == pTexture->LockRect( 0, &lr, NULL, 0 ))
	{
		// translate RGB->YUV2
		DWORD strideScreen=lr.Pitch;
		for (DWORD y=0; y < dwHeight; y++)
		{
			BYTE *pDest = (BYTE*)lr.pBits + strideScreen*y;
			for (DWORD x=0; x < (dwWidth>>1); x++)
			{
				RGBQUAD rgb1=image.GetPixelColor( (x<<1), y);
				RGBQUAD yuv1=image.RGBtoYUV(rgb1);
				RGBQUAD rgb2=image.GetPixelColor((x<<1)+1,y);
				RGBQUAD yuv2=image.RGBtoYUV(rgb2);
				BYTE Y0 = yuv1.rgbRed;
				BYTE U = (yuv1.rgbGreen+yuv2.rgbGreen)>>1;
				BYTE Y1 = yuv2.rgbRed;
				BYTE V = (yuv1.rgbBlue+yuv2.rgbBlue)>>1;

				*pDest++ = Y0;
				*pDest++ = U;
				*pDest++ = Y1;
				*pDest++ = V;
			}
		}
		pTexture->UnlockRect( 0 );
	}
	return pTexture;
}

bool CPicture::CreateAlbumThumbnail(const CStdString& strFileName, const CStdString& strAlbum)
{
  CStdString strThumbnail;
	CUtil::GetAlbumThumb(strAlbum, strThumbnail,true);
	return DoCreateThumbnail(strFileName, strThumbnail, MAX_ALBUM_THUMB_WIDTH, MAX_ALBUM_THUMB_HEIGHT);
}

bool CPicture::CreateThumnail(const CStdString& strFileName)
{
  CStdString strThumbnail;
  CUtil::GetThumbnail(strFileName, strThumbnail);
  bool bCacheFile=CUtil::IsRemote(strFileName);
	return DoCreateThumbnail(strFileName, strThumbnail, MAX_THUMB_WIDTH, MAX_THUMB_HEIGHT,bCacheFile);
}

bool CPicture::DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, int nMaxWidth, int nMaxHeight, bool bCacheFile/*=true*/)
{
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType=CXIMAGE_FORMAT_JPG;
	
	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return false;

	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tbn") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


	if (bCacheFile)
	{
		strCachedFile="T:\\cachedpic";
		strCachedFile+= strExtension;
		CFile file;
		if ( !file.Cache(strFileName.c_str(),strCachedFile.c_str(),NULL,NULL) )
		{
			CLog::Log("PICTURE::docreatethumbnail: Unable to cache file %s\n", strFileName.c_str());
			return false;
		}
	}
	else
		strCachedFile=strFileName;

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

  try
	{

		CxImage image(dwImageType);
		if (!image.Load(strCachedFile.c_str(),dwImageType))
		{
			CLog::Log("PICTURE::docreatethumbnail: Unable to open image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
			return false;
		}
		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();
	  
    bool bResize=false;
    float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);
		
    if (m_dwWidth > (DWORD)nMaxWidth )
    {
      bResize=true;
      m_dwWidth  = nMaxWidth;
      m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)nMaxHeight )
    {
      bResize=true;
      m_dwHeight =nMaxHeight;
      m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
			image.Resample(m_dwWidth,m_dwHeight, QUALITY);
			m_dwWidth=image.GetWidth();
			m_dwHeight=image.GetHeight();
		}

    ::DeleteFile(strThumbFileName.c_str());
    if ( image.GetNumColors() )
    {
      image.IncreaseBpp(24);
    }
		if (!image.Save(strThumbFileName.c_str(),CXIMAGE_FORMAT_JPG))
    {
			CLog::Log("PICTURE::docreatethumbnail: Unable to save image: %s Error:%s\n", strThumbFileName.c_str(), image.GetLastError());
      ::DeleteFile(strThumbFileName.c_str());
      return false;
    }
	}
  catch(...)
  {
			CLog::Log("PICTURE::docreatethumbnail: exception: %s\n", strCachedFile.c_str());
  }
	return true;
}

bool CPicture::CreateAlbumThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName)
{
	DWORD dwImageType=CXIMAGE_FORMAT_JPG;

	if (!strExtension.size()) return false;

	auto_aptr<BYTE> pPicture(new BYTE[nBufSize]);
	memcpy(pPicture.get(), pBuffer, nBufSize);
	
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"bitmap") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tbn") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),"pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

  try
	{
		CxImage image(dwImageType);
		if (!image.Decode(pPicture.get(),nBufSize,dwImageType))
		{
			CLog::Log("PICTURE::createthumbnailfrommemory: Unable to load image from memory Error:%s\n", image.GetLastError());
			return false;
		}
		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();
	  
    bool bResize=false;
    float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);

		if (m_dwWidth > (DWORD)MAX_ALBUM_THUMB_WIDTH )
    {
      bResize=true;
      m_dwWidth  = MAX_ALBUM_THUMB_WIDTH;
      m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)MAX_ALBUM_THUMB_HEIGHT )
    {
      bResize=true;
      m_dwHeight =MAX_ALBUM_THUMB_HEIGHT;
      m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
			image.Resample(m_dwWidth,m_dwHeight, QUALITY);
			m_dwWidth=image.GetWidth();
			m_dwHeight=image.GetHeight();
		}

    ::DeleteFile(strThumbFileName.c_str());
    if ( image.GetNumColors() )
    {
      image.IncreaseBpp(24);
    }
		if (!image.Save(strThumbFileName.c_str(),CXIMAGE_FORMAT_JPG))
    {
			CLog::Log("PICTURE::createthumbnailfrommemory: Unable to save image:%s Error:%s\n", strThumbFileName.c_str(), image.GetLastError());

      ::DeleteFile(strThumbFileName.c_str());
      return false;
    }
		return true;
	}
  catch(...)
  {
		CLog::Log("PICTURE::createthumbnailfrommemory: exception: memfile FileType: %s\n", strExtension.c_str());
  }
	return false;
}

void CPicture::RenderImage(IDirect3DTexture8* pTexture,int x, int y, int width, int height, int iTextureWidth, int iTextureHeight, bool bRGB)
{
  CPicture::VERTEX* vertex=NULL;
  LPDIRECT3DVERTEXBUFFER8 m_pVB;

	g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(CPicture::VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVB );
  m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

	float fx=(float)x;
	float fy=(float)y;
	float fwidth=(float)width;
	float fheight=(float)height;
	float fxOff = 0;
	float fyOff = 0;

  vertex[0].p = D3DXVECTOR4( fx - 0.5f,	fy - 0.5f,		0, 0 );
  vertex[0].tu = fxOff;
  vertex[0].tv = fyOff;

  vertex[1].p = D3DXVECTOR4( fx+fwidth - 0.5f,	fy - 0.5f,		0, 0 );
  vertex[1].tu = fxOff+(float)iTextureWidth;
  vertex[1].tv = fyOff;

  vertex[2].p = D3DXVECTOR4( fx+fwidth - 0.5f,	fy+fheight - 0.5f,	0, 0 );
  vertex[2].tu = fxOff+(float)iTextureWidth;
  vertex[2].tv = fyOff+(float)iTextureHeight;

  vertex[3].p = D3DXVECTOR4( fx - 0.5f,	fy+fheight - 0.5f,	0, 0 );
  vertex[3].tu = fxOff;
  vertex[3].tv = fyOff+iTextureHeight;
 
  vertex[0].col = 0xffffffff;
	vertex[1].col = 0xffffffff;
	vertex[2].col = 0xffffffff;
	vertex[3].col = 0xffffffff;
  m_pVB->Unlock();  


    // Set state to render the image
    g_graphicsContext.Get3DDevice()->SetTexture( 0, pTexture );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, bRGB ? FALSE : TRUE);
    g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
    // Render the image
    g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
		m_pVB->Release();
}



bool CPicture::Convert(const CStdString& strSource,const CStdString& strDest)
{
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType;
	
	CUtil::GetExtension(strSource,strExtension);
	if (!strExtension.size()) return false;

	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( 0==CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


  if (!CUtil::IsHD(strSource))
  {
	  strCachedFile="T:\\cachedpic";
	  strCachedFile+= strExtension;
	  CFile file;
	  if ( !file.Cache(strSource.c_str(),strCachedFile.c_str(),NULL,NULL) )
	  {
			CLog::Log("PICTURE::convert: Unable to cache image %s\n", strCachedFile.c_str());
      ::DeleteFile(strCachedFile.c_str());
		  return NULL;
	  }
  }
  else
  {
    strCachedFile=strSource;
  }

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

  try
	{
		CxImage image(dwImageType);
		if (!image.Load(strCachedFile.c_str(),dwImageType))
		{	
			CLog::Log("PICTURE::convert: Unable to load image: %s Error:%s\n", strCachedFile.c_str(), image.GetLastError());
			return false;
		}

    ::DeleteFile(strDest.c_str());		
    if ( image.GetNumColors() )
    {
      image.IncreaseBpp(24);
    }
		if (!image.Save(strDest.c_str(),CXIMAGE_FORMAT_JPG))
    {
			CLog::Log("PICTURE::convert: Unable to save image: %s Error:%s\n", strDest.c_str(), image.GetLastError());
      ::DeleteFile(strDest.c_str());
      return false;
    }
	}
  catch(...)
  {
			CLog::Log("PICTURE::convert: exception %s\n", strCachedFile.c_str());
    ::DeleteFile(strDest.c_str());
    return false;
  }

	return true;
}
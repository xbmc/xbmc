#include "picture.h"
#include "util.h"
#include "sectionloader.h"
#include "filesystem/file.h"

#define QUALITY 0

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

IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int iRotate)
{
	IDirect3DTexture8* pTexture=NULL;
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType;
	
	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return NULL;

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


	strCachedFile="T:\\cachedpic";
	strCachedFile+= strExtension;
	CFile file;
	if ( !file.Cache(strFileName.c_str(),strCachedFile.c_str()) )
	{
		return NULL;
	}

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

	{

		CxImage image(dwImageType);
		if (!image.Load(strCachedFile.c_str(),dwImageType))
		{
			
			return NULL;
		}
		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();
	  
    bool bResize=false;
    float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);
		
    if (m_dwWidth > (DWORD)g_graphicsContext.GetWidth() )
    {
      bResize=true;
      m_dwWidth  = g_graphicsContext.GetWidth();
      m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)g_graphicsContext.GetHeight() )
    {
      bResize=true;
      m_dwHeight =g_graphicsContext.GetHeight();
      m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
			image.Resample(m_dwWidth,m_dwHeight, QUALITY);
		}

    for (int i=0; i < iRotate; ++i) 
    {
      image.RotateRight();
    }
		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();


    bResize=false;
    fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);
		
    if (m_dwWidth > (DWORD)g_graphicsContext.GetWidth() )
    {
      bResize=true;
      m_dwWidth  = g_graphicsContext.GetWidth();
      m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)g_graphicsContext.GetHeight() )
    {
      bResize=true;
      m_dwHeight =g_graphicsContext.GetHeight();
      m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
			image.Resample(m_dwWidth,m_dwHeight, QUALITY);
		}

    m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();



		image.Flip();
		pTexture=GetTexture(image);
	}

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



bool CPicture::CreateThumnail(const CStdString& strFileName)
{
	CStdString strExtension;
	CStdString strCachedFile;
	DWORD dwImageType;
	
	CUtil::GetExtension(strFileName,strExtension);
	if (!strExtension.size()) return false;

	if ( CUtil::cmpnocase(strExtension.c_str(),".bmp") ) dwImageType=CXIMAGE_FORMAT_BMP;
	if ( CUtil::cmpnocase(strExtension.c_str(),".gif") ) dwImageType=CXIMAGE_FORMAT_GIF;
	if ( CUtil::cmpnocase(strExtension.c_str(),".jpg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( CUtil::cmpnocase(strExtension.c_str(),".jpeg") ) dwImageType=CXIMAGE_FORMAT_JPG;
	if ( CUtil::cmpnocase(strExtension.c_str(),".png") ) dwImageType=CXIMAGE_FORMAT_PNG;
	if ( CUtil::cmpnocase(strExtension.c_str(),".ico") ) dwImageType=CXIMAGE_FORMAT_ICO;
	if ( CUtil::cmpnocase(strExtension.c_str(),".tif") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( CUtil::cmpnocase(strExtension.c_str(),".tiff") ) dwImageType=CXIMAGE_FORMAT_TIF;
	if ( CUtil::cmpnocase(strExtension.c_str(),".tga") ) dwImageType=CXIMAGE_FORMAT_TGA;
	if ( CUtil::cmpnocase(strExtension.c_str(),".pcx") ) dwImageType=CXIMAGE_FORMAT_PCX;


	strCachedFile="T:\\cachedpic";
	strCachedFile+= strExtension;
	CFile file;
	if ( !file.Cache(strFileName.c_str(),strCachedFile.c_str()) )
	{
		return NULL;
	}

	if (!m_bSectionLoaded)
	{
		CSectionLoader::Load("CXIMAGE");
		m_bSectionLoaded=true;
	}

	{

		CxImage image(dwImageType);
		if (!image.Load(strCachedFile.c_str(),dwImageType))
		{
			
			return NULL;
		}
		m_dwWidth=image.GetWidth();
		m_dwHeight=image.GetHeight();
	  
    bool bResize=false;
    float fAspect= ((float)m_dwWidth) / ((float)m_dwHeight);
		
    if (m_dwWidth > (DWORD)64 )
    {
      bResize=true;
      m_dwWidth  = 64;
      m_dwHeight = (DWORD)( ( (float)m_dwWidth) / fAspect);
    }

    if (m_dwHeight > (DWORD)64 )
    {
      bResize=true;
      m_dwHeight =64;
      m_dwWidth  = (DWORD)(  fAspect * ( (float)m_dwHeight) );
    }

    if (bResize)
    {
			image.Resample(m_dwWidth,m_dwHeight, QUALITY);
			m_dwWidth=image.GetWidth();
			m_dwHeight=image.GetHeight();
		}

    CStdString strThumbnail;
    CUtil::GetThumbnail(strFileName, strThumbnail);
		
		image.Save(strThumbnail.c_str(),CXIMAGE_FORMAT_JPG);
	}

	return true;
}
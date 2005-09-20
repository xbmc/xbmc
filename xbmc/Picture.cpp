
#include "stdafx.h"
#include "picture.h"
#include "util.h"


#define QUALITY 0
#define MAX_THUMB_WIDTH 128
#define MAX_THUMB_HEIGHT 128

#define MAX_ALBUM_THUMB_WIDTH 128
#define MAX_ALBUM_THUMB_HEIGHT 128

CPicture::CPicture(void)
{
  ZeroMemory(&m_info, sizeof(ImageInfo));
}

CPicture::~CPicture(void)
{

}

IDirect3DTexture8* CPicture::Load(const CStdString& strFileName, int iMaxWidth, int iMaxHeight)
{
  if (!m_dll.Load()) return NULL;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.LoadImage(strFileName.c_str(), iMaxWidth, iMaxHeight, &m_info))
  {
    CLog::Log(LOGERROR, "PICTURE: Error loading image %s", strFileName.c_str());
    return NULL;
  }
  // loaded successfully
  return m_info.texture;
}

bool CPicture::CreateAlbumThumbnail(const CStdString& strFileName, const CStdString& strAlbum)
{
  CStdString strThumbnail;
  CUtil::GetAlbumFolderThumb(strAlbum, strThumbnail, true);
  return DoCreateThumbnail(strFileName, strThumbnail);
}

bool CPicture::CreateThumbnail(const CStdString& strFileName)
{
  CStdString strThumbnail;
  CUtil::GetThumbnail(strFileName, strThumbnail);
  return DoCreateThumbnail(strFileName, strThumbnail);
}

bool CPicture::DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName)
{
  // don't create the thumb if it already exists
  if (CFile::Exists(strThumbFileName))
    return true;

  CLog::Log(LOGINFO, "Creating thumb from: %s as: %s", strFileName.c_str(),strThumbFileName.c_str());
  
  // load our dll
  if (!m_dll.Load()) return false;

  memset(&m_info, 0, sizeof(ImageInfo));
  if (!m_dll.CreateThumbnail(strFileName.c_str(), strThumbFileName.c_str()))
  {
    CLog::Log(LOGERROR, "PICTURE::DoCreateThumbnail: Unable to create thumbfile %s from image %s", strThumbFileName.c_str(), strFileName.c_str());
    return false;
  }
  return true;
}

bool CPicture::CreateAlbumThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName)
{
  CLog::Log(LOGINFO, "Creating album thumb from memory: %s", strThumbFileName.c_str());
  if (!m_dll.Load()) return false;
  if (!m_dll.CreateThumbnailFromMemory((BYTE *)pBuffer, nBufSize, strExtension.c_str(), strThumbFileName.c_str()))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateAlbumThumbnailFromMemory: exception: memfile FileType: %s\n", strExtension.c_str());
    return false;
  }
  return true;
}

void CPicture::RenderImage(IDirect3DTexture8* pTexture, float x, float y, float width, float height, int iTextureWidth, int iTextureHeight, int iTextureLeft, int iTextureTop, DWORD dwAlpha)
{
  CPicture::VERTEX* vertex = NULL;
  LPDIRECT3DVERTEXBUFFER8 m_pVB;

  g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(CPicture::VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVB );
  m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

  float fxOff = (float)iTextureLeft;
  float fyOff = (float)iTextureTop;

  vertex[0].p = D3DXVECTOR4( x - 0.5f, y - 0.5f, 0, 0 );
  vertex[0].tu = fxOff;
  vertex[0].tv = fyOff;

  vertex[1].p = D3DXVECTOR4( x + width - 0.5f, y - 0.5f, 0, 0 );
  vertex[1].tu = fxOff + (float)iTextureWidth;
  vertex[1].tv = fyOff;

  vertex[2].p = D3DXVECTOR4( x + width - 0.5f, y + height - 0.5f, 0, 0 );
  vertex[2].tu = fxOff + (float)iTextureWidth;
  vertex[2].tv = fyOff + (float)iTextureHeight;

  vertex[3].p = D3DXVECTOR4( x - 0.5f, y + height - 0.5f, 0, 0 );
  vertex[3].tu = fxOff;
  vertex[3].tv = fyOff + iTextureHeight;

  D3DCOLOR dwColor = (dwAlpha << 24) | 0xFFFFFF;
  for (int i = 0; i < 4; i++)
  {
    vertex[i].col = dwColor;
  }
  m_pVB->Unlock();

  // Set state to render the image
  g_graphicsContext.Get3DDevice()->SetTexture( 0, pTexture );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR /*g_stSettings.m_minFilter*/ );
  g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR /*g_stSettings.m_maxFilter*/ );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE, FALSE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE, FALSE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, FALSE);
  g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
  // Render the image
  g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
  g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
  // reset texture state
  g_graphicsContext.Get3DDevice()->SetTexture(0, NULL);
  m_pVB->Release();
}

bool CPicture::Convert(const CStdString& strSource, const CStdString& strDest)
{
  return DoCreateThumbnail(strSource, strDest);
}

void CPicture::CreateFolderThumb(CStdString &strFolder, CStdString *strThumbs)
{ // we want to mold the thumbs together into one single one
  if (!m_dll.Load()) return;
  CStdString strFolderThumbnail;
  CUtil::GetThumbnail(strFolder, strFolderThumbnail);
  if (CFile::Exists(strFolderThumbnail))
    return;
  CStdString strThumbnails[4];
  const char *szThumbs[4];
  for (int i=0; i < 4; i++)
  {
    if (strThumbs[i].IsEmpty())
      strThumbnails[i] = "";
    else
    {
      CreateThumbnail(strThumbs[i]);
      CUtil::GetThumbnail(strThumbs[i], strThumbnails[i]);
    }
    szThumbs[i] = strThumbnails[i].c_str();
  }
  if (!m_dll.CreateFolderThumbnail(szThumbs, strFolderThumbnail.c_str()))
  {
    CLog::Log(LOGERROR, "PICTURE::CreateFolderThumb() failed for folder %s", strFolder.c_str());
  }
}

bool CPicture::CreateExifThumbnail(const CStdString &strFile, const CStdString &strCachedThumb)
{
  if (!m_dll.Load()) return false;
  return m_dll.CreateExifThumbnail(strFile.c_str(), strCachedThumb.c_str());
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

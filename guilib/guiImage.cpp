#include "stdafx.h"
#include "GUIImage.h"
#include "TextureManager.h"
#include "../xbmc/settings.h"



CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture,DWORD dwColorKey)
:CGUIControl(dwParentID, dwControlId,iPosX, iPosY, dwWidth, dwHeight)
{
  m_colDiffuse	= 0xFFFFFFFF;  
  
  m_strFileName=strTexture;
  m_iTextureWidth=0;
  m_iTextureHeight=0;
  m_dwColorKey=dwColorKey;
  m_iBitmap=0;
  m_dwItems=1;
	m_iCurrentImage=0;
	m_dwFrameCounter=-1;
  m_bKeepAspectRatio=false;
  m_iCurrentLoop=0;
  m_iRenderWidth=dwWidth;
  m_iRenderHeight=dwHeight;
	m_iImageWidth = 0;
	m_iImageHeight = 0;
	m_bWasVisible = m_bVisible;
	for (int i=0; i<4; i++)
		m_dwAlpha[i] = 0xFF;
	ControlType = GUICONTROL_IMAGE;
}

CGUIImage::CGUIImage(const CGUIImage &left)
:CGUIControl(left)
{
  m_colDiffuse	    = left.m_colDiffuse;
  m_strFileName     = left.m_strFileName;
  m_dwColorKey      = left.m_dwColorKey;
  m_bKeepAspectRatio= left.m_bKeepAspectRatio;
  m_iRenderWidth    = left.m_iRenderWidth;
  m_iRenderHeight   = left.m_iRenderHeight;
  // defaults
	m_iCurrentImage=0;
  m_iBitmap=0;
  m_dwItems=1;
	m_dwFrameCounter=-1;
  m_iCurrentLoop=0;
	m_iImageWidth = 0;
	m_iImageHeight = 0;
  m_iTextureWidth=0;
  m_iTextureHeight=0;
	for (int i=0; i<4; i++)
		m_dwAlpha[i] = left.m_dwAlpha[i];
  m_pPalette = NULL;
	ControlType = GUICONTROL_IMAGE;
}

CGUIImage::~CGUIImage(void)
{
}

void CGUIImage::Render(int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
{
  if (m_vecTextures.size()==0) return;
  // save old position + size
  int oldPosX = m_iPosX;
  int oldPosY = m_iPosY;
  DWORD oldWidth = m_dwWidth;
  DWORD oldHeight = m_dwHeight;
  SetPosition(iPosX, iPosY);
  SetWidth(dwWidth);
  SetHeight(dwHeight);
	Render();
  // reset old position + size
  SetPosition(oldPosX, oldPosY);
  SetWidth(oldWidth);
  SetHeight(oldHeight);
}

void CGUIImage::Render()
{
  if (!m_bVisible)
  {
    m_bWasVisible = false;
    return;
  }
  if (!m_vecTextures.size())
    return ;
  
  Process();
  if (m_bInvalidated) UpdateVB();

  LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
  // Set state to render the image
#ifdef ALLOW_TEXTURE_COMPRESSION
  p3DDevice->SetPalette( 0, m_pPalette);
#endif
  p3DDevice->SetTexture( 0, m_vecTextures[m_iCurrentImage] );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
  p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );

  p3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE );
  p3DDevice->SetRenderState( D3DRS_ALPHAREF,         0 );
  p3DDevice->SetRenderState( D3DRS_ALPHAFUNC,        D3DCMP_GREATEREQUAL );
  p3DDevice->SetRenderState( D3DRS_ZENABLE,      FALSE );
  p3DDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
  p3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
  p3DDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
  p3DDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
  p3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  p3DDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
  p3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  p3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE);
  p3DDevice->SetVertexShader( FVF_VERTEX );
  // Render the image
  p3DDevice->Begin(D3DPT_QUADLIST);

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs, 0.0f );
  D3DCOLOR color = m_colDiffuse;
  if (m_dwAlpha[0] != 0xFF) color = (m_dwAlpha[0] << 24) | (m_colDiffuse & 0x00FFFFFF);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, color);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, m_fX - 0.5f, m_fY - 0.5f, 0.0f, 0.0f );

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs + m_fU, 0.0f );
  color = m_colDiffuse;
  if (m_dwAlpha[1] != 0xFF) color = (m_dwAlpha[1] << 24) | (m_colDiffuse & 0x00FFFFFF);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, color);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, m_fX + m_fNW - 0.5f, m_fY - 0.5f, 0.0f, 0.0f );

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs + m_fU, m_fV );
  color = m_colDiffuse;
  if (m_dwAlpha[2] != 0xFF) color = (m_dwAlpha[2] << 24) | (m_colDiffuse & 0x00FFFFFF);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, color);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, m_fX + m_fNW - 0.5f, m_fY + m_fNH - 0.5f, 0.0f, 0.0f );

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs, m_fV );
  color = m_colDiffuse;
  if (m_dwAlpha[3] != 0xFF) color = (m_dwAlpha[3] << 24) | (m_colDiffuse & 0x00FFFFFF);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, color);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, m_fX - 0.5f, m_fY + m_fNH - 0.5f, 0.0f, 0.0f );

  p3DDevice->End();

  // unset the texture and palette or the texture caching crashes because the runtime still has a reference
  p3DDevice->SetTexture( 0, NULL);
#ifdef ALLOW_TEXTURE_COMPRESSION
  p3DDevice->SetPalette( 0, NULL);
#endif
  CGUIControl::Render();
}

void CGUIImage::OnAction(const CAction &action) 
{
}

bool CGUIImage::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIImage::PreAllocResources()
{
	FreeResources();
	g_TextureManager.PreLoad(m_strFileName);
}

void CGUIImage::AllocResources()
{
  CGUIControl::AllocResources();
  FreeResources();

	m_dwFrameCounter=0;
	m_iCurrentImage=0;
  m_iCurrentLoop=0;

  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey);
  if (!iImages) return;
  for (int i=0; i < iImages; i++)
  {
    LPDIRECT3DTEXTURE8 pTexture;
		pTexture=g_TextureManager.GetTexture(m_strFileName,i, m_iTextureWidth,m_iTextureHeight,m_pPalette);
    m_vecTextures.push_back(pTexture);
  }

  // Set state to render the image
  UpdateVB();
}

void CGUIImage::FreeResources()
{
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    g_TextureManager.ReleaseTexture(m_strFileName,i);
  }

  m_vecTextures.erase(m_vecTextures.begin(),m_vecTextures.end());
	m_iCurrentImage = 0;
  m_iCurrentLoop  = 0;
	m_iImageWidth   = 0;
	m_iImageHeight  = 0;
}

void CGUIImage::Update()
{
  CGUIControl::Update();
}

void CGUIImage::UpdateVB()
{
  if (m_vecTextures.size()==0) return;

  m_fX = (float)m_iPosX;
  m_fY = (float)m_iPosY;
#ifdef ALLOW_TEXTURE_COMPRESSION
	if (0==m_iImageWidth|| 0==m_iImageHeight)
	{
		D3DSURFACE_DESC desc;
		m_vecTextures[m_iCurrentImage]->GetLevelDesc(0,&desc);

		m_iImageWidth = desc.Width;
		m_iImageHeight = desc.Height;
	}

  if (0==m_iTextureWidth|| 0==m_iTextureHeight)
  {
	  m_iTextureWidth  = m_iImageWidth/m_dwItems;
		m_iTextureHeight = m_iImageHeight;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#else
  D3DSURFACE_DESC desc;
  m_vecTextures[m_iCurrentImage]->GetLevelDesc(0,&desc);

  if (0==m_iTextureWidth|| 0==m_iTextureHeight)
  {
	  m_iTextureWidth  = (DWORD) desc.Width/m_dwItems;
	  m_iTextureHeight = (DWORD) desc.Height;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#endif
  if (m_dwWidth && m_dwItems>1)
  {
    m_iTextureWidth=m_dwWidth;
  }

  if (m_dwWidth==0) 
    m_dwWidth=m_iTextureWidth;
  if (m_dwHeight==0) 
    m_dwHeight=m_iTextureHeight;


  m_fNW = (float)m_dwWidth;
  m_fNH = (float)m_dwHeight;

  if (m_bKeepAspectRatio && m_iTextureWidth && m_iTextureHeight)
  {
    RESOLUTION iResolution = g_graphicsContext.GetVideoResolution();
    float fSourceFrameRatio = ((float)m_iTextureWidth) / ((float)m_iTextureHeight);
    float fOutputFrameRatio = fSourceFrameRatio / g_graphicsContext.GetPixelRatio(iResolution); 

    // maximize the thumbnails width
    float fNewWidth  = (float)m_dwWidth;
    float fNewHeight = fNewWidth/fOutputFrameRatio;

    if (fNewHeight > m_dwHeight)
    {
      fNewHeight = (float)m_dwHeight;
      fNewWidth = fNewHeight*fOutputFrameRatio;
    }
    // this shouldnt happen, but just make sure that everything still fits onscreen
    if (fNewWidth > m_dwWidth || fNewHeight > m_dwHeight)
    {
      fNewWidth  = (float)m_dwWidth;
      fNewHeight = (float)m_dwHeight;
    }
    m_fNW = fNewWidth;
    m_fNH = fNewHeight;
  }


  m_iRenderWidth  = (int)m_fNW;
  m_iRenderHeight = (int)m_fNH;

	if (CalibrationEnabled())
	{
		g_graphicsContext.Correct(m_fX, m_fY);
	}

#ifdef ALLOW_TEXTURE_COMPRESSION
	m_fUOffs = float(m_iBitmap * m_dwWidth) / float(m_iImageWidth);
	m_fU     = float(m_iTextureWidth) / float(m_iImageWidth);
	m_fV     = float(m_iTextureHeight) / float(m_iImageHeight);
#else
  m_fUOffs = float(m_iBitmap*m_dwWidth);
  m_fU     = float(m_iTextureWidth);
  m_fV     = float(m_iTextureHeight);
#endif
}

bool CGUIImage::CanFocus() const
{
  return false;
}

void CGUIImage::Select(int iBitmap)
{
  if (m_iBitmap != iBitmap)
  {
    m_iBitmap=iBitmap;
    Update();
    m_bInvalidated = true;
  }
}

void CGUIImage::SetItems(int iItems)
{
  m_dwItems=iItems;
}

void CGUIImage::Process()
{
	if (m_vecTextures.size() <= 1)
		return;

	if (!m_bWasVisible)
	{
		m_iCurrentLoop = 0;
		m_iCurrentImage = 0;
		m_dwFrameCounter = 0;
		m_bWasVisible = true;
		return;
	}

	m_dwFrameCounter++;
  DWORD dwDelay    = g_TextureManager.GetDelay(m_strFileName,m_iCurrentImage);
  int   iMaxLoops  = g_TextureManager.GetLoops(m_strFileName,m_iCurrentImage);
	if (!dwDelay) dwDelay=100;
	if (m_dwFrameCounter*40 >= dwDelay)
	{
		m_dwFrameCounter=0;
    if (m_iCurrentImage+1 >= (int)m_vecTextures.size() )
		{
      if (iMaxLoops > 0)
      {
        if (m_iCurrentLoop+1 < iMaxLoops)
        {
          m_iCurrentLoop++;
			    m_iCurrentImage=0;
        }
      }
      else
      {
        // 0 == loop forever
			  m_iCurrentImage=0;
      }
		}
    else
    {
		  m_iCurrentImage++;
    }
	}
}
void CGUIImage::SetTextureWidth(int iWidth)
{
  if (m_iTextureWidth != iWidth)
  {
	  m_iTextureWidth=iWidth;
	  Update();
    m_bInvalidated = true;
  }
}
void CGUIImage::SetTextureHeight(int iHeight)
{
  if (m_iTextureHeight != iHeight)
  {
	  m_iTextureHeight=iHeight;
	  Update();
    m_bInvalidated = true;
  }
}
int	CGUIImage::GetTextureWidth() const
{
	return m_iTextureWidth;
}
int CGUIImage::GetTextureHeight() const
{
	return m_iTextureHeight;
}

void CGUIImage::SetKeepAspectRatio(bool bOnOff)
{
  if (m_bKeepAspectRatio != bOnOff)
  {
    m_bKeepAspectRatio=bOnOff;
    m_bInvalidated = true;
  }
}
bool CGUIImage::GetKeepAspectRatio() const
{
  return m_bKeepAspectRatio;
}


int CGUIImage::GetRenderWidth() const
{
  return m_iRenderWidth;
}
int CGUIImage::GetRenderHeight() const
{
  return m_iRenderHeight;
}

void CGUIImage::SetFileName(const CStdString& strFileName)
{
  FreeResources();
  m_strFileName = strFileName;
  AllocResources();
}

void CGUIImage::SetCornerAlpha(DWORD dwLeftTop, DWORD dwRightTop, DWORD dwLeftBottom, DWORD dwRightBottom)
{
  if (
	  m_dwAlpha[0] != dwLeftTop ||
	  m_dwAlpha[1] != dwRightTop ||
	  m_dwAlpha[2] != dwLeftBottom ||
	  m_dwAlpha[3] != dwRightBottom
  )
  {
	  m_dwAlpha[0] = dwLeftTop;
	  m_dwAlpha[1] = dwRightTop;
	  m_dwAlpha[2] = dwLeftBottom;
	  m_dwAlpha[3] = dwRightBottom;
    m_bInvalidated = true;
  }
}


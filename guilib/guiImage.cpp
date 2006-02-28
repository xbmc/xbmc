#include "include.h"
#include "GUIImage.h"
#include "TextureManager.h"
#include "../xbmc/settings.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture, DWORD dwColorKey)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_colDiffuse = 0xFFFFFFFF;

  m_strFileName = strTexture;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  m_dwColorKey = dwColorKey;
  m_iBitmap = 0;
  m_dwItems = 1;
  m_iCurrentImage = 0;
  m_dwFrameCounter = -1;
  m_aspectRatio = ASPECT_RATIO_STRETCH;
  m_iCurrentLoop = 0;
  m_iRenderWidth = dwWidth;
  m_iRenderHeight = dwHeight;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_bWasVisible = m_bVisible;
  for (int i = 0; i < 4; i++)
    m_dwAlpha[i] = 0xFF;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_Info = 0;
}

CGUIImage::CGUIImage(const CGUIImage &left)
    : CGUIControl(left)
{
  m_colDiffuse = left.m_colDiffuse;
  m_strFileName = left.m_strFileName;
  m_dwColorKey = left.m_dwColorKey;
  m_aspectRatio = left.m_aspectRatio;
  m_iRenderWidth = left.m_iRenderWidth;
  m_iRenderHeight = left.m_iRenderHeight;
  // defaults
  m_iCurrentImage = 0;
  m_iBitmap = 0;
  m_dwItems = 1;
  m_dwFrameCounter = -1;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  for (int i = 0; i < 4; i++)
    m_dwAlpha[i] = left.m_dwAlpha[i];
  m_pPalette = NULL;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_Info = left.m_Info;
}

CGUIImage::~CGUIImage(void)
{

}

void CGUIImage::Render(int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
{
  if (m_vecTextures.size() == 0) return ;
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
  bool bVisible = IsVisible();

  // check for conditional information before we free and
  // alloc as this does free and allocation as well
  if (m_Info)
  {
    CStdString strImage = g_infoManager.GetImage(m_Info);
    if (strImage != m_strFileName)
      SetFileName(strImage);
  }

  if (m_bDynamicResourceAlloc && !bVisible && IsAllocated())
    FreeResources();

  if (!bVisible)
  {
    m_bWasVisible = false;
    return;
  }

  if (m_bDynamicResourceAlloc && !IsAllocated())
    AllocResources();
  else if (!m_bDynamicResourceAlloc && !IsAllocated())
    AllocResources();  // not dynamic, make sure we allocate!

  if (m_vecTextures.size())
  {
    Process();
    if (m_bInvalidated) UpdateVB();
    // scale to screen output position
    if (m_fNW > m_dwWidth || m_fNH > m_dwHeight)
      if (!g_graphicsContext.SetViewPort((float)m_iPosX, (float)m_iPosY, (float)m_dwWidth, (float)m_dwHeight, true))
        return;
    float x1 = floor(g_graphicsContext.ScaleFinalXCoord(m_fX, m_fY) + 0.5f) - 0.5f;
    float y1 = floor(g_graphicsContext.ScaleFinalYCoord(m_fX, m_fY) + 0.5f) - 0.5f;
    float x2 = floor(g_graphicsContext.ScaleFinalXCoord(m_fX + m_fNW, m_fY) + 0.5f) - 0.5f;
    float y2 = floor(g_graphicsContext.ScaleFinalYCoord(m_fX + m_fNW, m_fY) + 0.5f) - 0.5f;
    float x3 = floor(g_graphicsContext.ScaleFinalXCoord(m_fX + m_fNW, m_fY + m_fNH) + 0.5f) - 0.5f;
    float y3 = floor(g_graphicsContext.ScaleFinalYCoord(m_fX + m_fNW, m_fY + m_fNH) + 0.5f) - 0.5f;
    float x4 = floor(g_graphicsContext.ScaleFinalXCoord(m_fX, m_fY + m_fNH) + 0.5f) - 0.5f;
    float y4 = floor(g_graphicsContext.ScaleFinalYCoord(m_fX, m_fY + m_fNH) + 0.5f) - 0.5f;

    if (y3 == y1) y3 += 1.0f; if (x2 == x1) x2 += 1.0f;
    if (y4 == y2) y4 += 1.0f; if (x3 == x4) x3 += 1.0f;
    LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
    // Set state to render the image
#ifdef ALLOW_TEXTURE_COMPRESSION
    p3DDevice->SetPalette( 0, m_pPalette);
#endif
    p3DDevice->SetTexture( 0, m_vecTextures[m_iCurrentImage] );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

    p3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
    p3DDevice->SetRenderState( D3DRS_ALPHAREF, 0 );
    p3DDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
    p3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    p3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    p3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    p3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    p3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    p3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    p3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    p3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE);
    p3DDevice->SetVertexShader( FVF_VERTEX );

    // Render the image
    p3DDevice->Begin(D3DPT_QUADLIST);

    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs, 0.0f);
    D3DCOLOR color = m_colDiffuse;
    if (m_dwAlpha[0] != 0xFF) color = (m_dwAlpha[0] << 24) | (m_colDiffuse & 0x00FFFFFF);
    p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
    p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x1, y1, 0, 0 );

    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs + m_fU, 0.0f);
    color = m_colDiffuse;
    if (m_dwAlpha[1] != 0xFF) color = (m_dwAlpha[1] << 24) | (m_colDiffuse & 0x00FFFFFF);
    p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
    p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x2, y2, 0, 0 );

    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs + m_fU, m_fV);
    color = m_colDiffuse;
    if (m_dwAlpha[2] != 0xFF) color = (m_dwAlpha[2] << 24) | (m_colDiffuse & 0x00FFFFFF);
    p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
    p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x3, y3, 0, 0 );

    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, m_fUOffs, m_fV);
    color = m_colDiffuse;
    if (m_dwAlpha[3] != 0xFF) color = (m_dwAlpha[3] << 24) | (m_colDiffuse & 0x00FFFFFF);
    p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
    p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x4, y4, 0, 0 );

    p3DDevice->End();

    // unset the texture and palette or the texture caching crashes because the runtime still has a reference
    p3DDevice->SetTexture( 0, NULL);
#ifdef ALLOW_TEXTURE_COMPRESSION
    p3DDevice->SetPalette( 0, NULL);
#endif
    if (m_fNW > m_dwWidth || m_fNH > m_dwHeight)
      g_graphicsContext.RestoreViewPort();
  }
  CGUIControl::Render();
}

bool CGUIImage::OnAction(const CAction &action)
{
  return false;
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
  if (m_strFileName.IsEmpty())
    return;
  FreeResources();
  CGUIControl::AllocResources();

  m_dwFrameCounter = 0;
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;

  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey);
  if (!iImages) return ;
  for (int i = 0; i < iImages; i++)
  {
    LPDIRECT3DTEXTURE8 pTexture;
    pTexture = g_TextureManager.GetTexture(m_strFileName, i, m_iTextureWidth, m_iTextureHeight, m_pPalette);
    m_vecTextures.push_back(pTexture);
  }

  // Set state to render the image
  UpdateVB();
}

void CGUIImage::FreeResources()
{
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    g_TextureManager.ReleaseTexture(m_strFileName, i);
  }

  m_vecTextures.erase(m_vecTextures.begin(), m_vecTextures.end());
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;

  CGUIControl::FreeResources();
}

void CGUIImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

void CGUIImage::Update()
{
  CGUIControl::Update();
}

void CGUIImage::UpdateVB()
{
  if (m_vecTextures.size() == 0) return ;

  m_fX = (float)m_iPosX;
  m_fY = (float)m_iPosY;
#ifdef ALLOW_TEXTURE_COMPRESSION
  if (0 == m_iImageWidth || 0 == m_iImageHeight)
  {
    D3DSURFACE_DESC desc;
    m_vecTextures[m_iCurrentImage]->GetLevelDesc(0, &desc);

    m_iImageWidth = desc.Width;
    m_iImageHeight = desc.Height;
  }

  if (0 == m_iTextureWidth || 0 == m_iTextureHeight)
  {
    m_iTextureWidth = m_iImageWidth / m_dwItems;
    m_iTextureHeight = m_iImageHeight;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
      m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
      m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#else
  D3DSURFACE_DESC desc;
  m_vecTextures[m_iCurrentImage]->GetLevelDesc(0, &desc);

  if (0 == m_iTextureWidth || 0 == m_iTextureHeight)
  {
    m_iTextureWidth = (DWORD) desc.Width / m_dwItems;
    m_iTextureHeight = (DWORD) desc.Height;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
      m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
      m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#endif
  if (m_dwWidth && m_dwItems > 1)
  {
    m_iTextureWidth = m_dwWidth;
  }

  if (m_dwWidth == 0)
    m_dwWidth = m_iTextureWidth;
  if (m_dwHeight == 0)
    m_dwHeight = m_iTextureHeight;


  m_fNW = (float)m_dwWidth;
  m_fNH = (float)m_dwHeight;

  if (m_aspectRatio != ASPECT_RATIO_STRETCH && m_iTextureWidth && m_iTextureHeight)
  {
    float fSourceFrameRatio = ((float)m_iTextureWidth) / ((float)m_iTextureHeight);
    float fOutputFrameRatio = fSourceFrameRatio / g_graphicsContext.GetPixelRatio(g_graphicsContext.GetVideoResolution());

    // maximize the thumbnails width
    float fNewWidth = (float)m_dwWidth;
    float fNewHeight = fNewWidth / fOutputFrameRatio;

    if ((m_aspectRatio == CGUIImage::ASPECT_RATIO_SCALE && fNewHeight < m_dwHeight) ||
        (m_aspectRatio == CGUIImage::ASPECT_RATIO_KEEP && fNewHeight > m_dwHeight))
    {
      fNewHeight = (float)m_dwHeight;
      fNewWidth = fNewHeight * fOutputFrameRatio;
    }
    m_fNW = fNewWidth;
    m_fNH = fNewHeight;
    m_fX = m_iPosX - (fNewWidth - m_dwWidth) * 0.5f;
    m_fY = m_iPosY - (fNewHeight - m_dwHeight) * 0.5f;
  }


  m_iRenderWidth = (int)m_fNW;
  m_iRenderHeight = (int)m_fNH;

#ifdef ALLOW_TEXTURE_COMPRESSION
  m_fUOffs = float(m_iBitmap * m_dwWidth) / float(m_iImageWidth);
  m_fU = float(m_iTextureWidth) / float(m_iImageWidth);
  m_fV = float(m_iTextureHeight) / float(m_iImageHeight);
#else
  m_fUOffs = float(m_iBitmap * m_dwWidth);
  m_fU = float(m_iTextureWidth);
  m_fV = float(m_iTextureHeight);
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
    m_iBitmap = iBitmap;
    Update();
    m_bInvalidated = true;
  }
}

void CGUIImage::SetItems(int iItems)
{
  m_dwItems = iItems;
}

void CGUIImage::Process()
{
  if (m_vecTextures.size() <= 1)
    return ;

  if (!m_bWasVisible)
  {
    m_iCurrentLoop = 0;
    m_iCurrentImage = 0;
    m_dwFrameCounter = 0;
    m_bWasVisible = true;
    return ;
  }

  m_dwFrameCounter++;
  DWORD dwDelay = g_TextureManager.GetDelay(m_strFileName, m_iCurrentImage);
  int iMaxLoops = g_TextureManager.GetLoops(m_strFileName, m_iCurrentImage);
  if (!dwDelay) dwDelay = 100;
  if (m_dwFrameCounter*40 >= dwDelay)
  {
    m_dwFrameCounter = 0;
    if (m_iCurrentImage + 1 >= (int)m_vecTextures.size() )
    {
      if (iMaxLoops > 0)
      {
        if (m_iCurrentLoop + 1 < iMaxLoops)
        {
          m_iCurrentLoop++;
          m_iCurrentImage = 0;
        }
      }
      else
      {
        // 0 == loop forever
        m_iCurrentImage = 0;
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
    m_iTextureWidth = iWidth;
    Update();
    m_bInvalidated = true;
  }
}

void CGUIImage::SetTextureHeight(int iHeight)
{
  if (m_iTextureHeight != iHeight)
  {
    m_iTextureHeight = iHeight;
    Update();
    m_bInvalidated = true;
  }
}

int CGUIImage::GetTextureWidth() const
{
  return m_iTextureWidth;
}

int CGUIImage::GetTextureHeight() const
{
  return m_iTextureHeight;
}

void CGUIImage::SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio)
{
  if (m_aspectRatio != ratio)
  {
    m_aspectRatio = ratio;
    m_bInvalidated = true;
  }
}

CGUIImage::GUIIMAGE_ASPECT_RATIO CGUIImage::GetAspectRatio() const
{
  return m_aspectRatio;
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
  if (m_strFileName.Equals(strFileName)) return;
  FreeResources();
  m_strFileName = strFileName;
  // Don't allocate resources here as this is done at render time
//  AllocResources();
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

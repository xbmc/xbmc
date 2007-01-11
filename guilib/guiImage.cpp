#include "include.h"
#include "GUIImage.h"
#include "TextureManager.h"
#include "../xbmc/settings.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, DWORD dwColorKey)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_colDiffuse = 0xFFFFFFFF;
  m_strFileName = texture.file;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  m_dwColorKey = dwColorKey;
  m_iCurrentImage = 0;
  m_dwFrameCounter = -1;
  m_aspectRatio = ASPECT_RATIO_STRETCH;
  m_iCurrentLoop = 0;
  m_renderWidth = width;
  m_renderHeight = height;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_bWasVisible = m_visible;
  m_dwAlpha = 0xFF;
  for (int i = 0; i < 4; i++)
    m_cornerAlpha[i] = 0xFF;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
  m_Info = 0;
  m_image = texture;
}

CGUIImage::CGUIImage(const CGUIImage &left)
    : CGUIControl(left)
{
  m_colDiffuse = left.m_colDiffuse;
  m_strFileName = left.m_strFileName;
  m_dwColorKey = left.m_dwColorKey;
  m_aspectRatio = left.m_aspectRatio;
  m_renderWidth = left.m_renderWidth;
  m_renderHeight = left.m_renderHeight;
  // defaults
  m_iCurrentImage = 0;
  m_dwFrameCounter = -1;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  m_dwAlpha = left.m_dwAlpha;
  for (int i = 0; i < 4; i++)
    m_cornerAlpha[i] = left.m_cornerAlpha[i];
  m_pPalette = NULL;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
  m_Info = left.m_Info;
  m_image = left.m_image;
}

CGUIImage::~CGUIImage(void)
{

}

void CGUIImage::Render()
{
  bool bVisible = IsVisible();

  // check for conditional information before we free and
  // alloc as this does free and allocation as well
  if (m_Info)
  {
    SetFileName(g_infoManager.GetImage(m_Info, m_dwParentID));
  }

  if (m_bDynamicResourceAlloc && !bVisible && IsAllocated())
    FreeResources();

  if (!bVisible)
  {
    m_bWasVisible = false;
    return;
  }

  // only check m_texturesAllocated if we are dynamicm, as we only want to check
  // for unavailable textures once
  if (m_bDynamicResourceAlloc && !m_texturesAllocated)
    AllocResources();
  else if (!m_bDynamicResourceAlloc && !m_texturesAllocated)
    AllocResources();  // not dynamic, make sure we allocate!

  if (m_vecTextures.size())
  {
    Process();
    if (m_bInvalidated) CalculateSize();
    // scale to screen output position
    if (m_fNW > m_width || m_fNH > m_height)
    {
      if (!g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height, true))
      {
        CGUIControl::Render();
        return;
      }
    }

    LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
    // Set state to render the image
#ifdef ALLOW_TEXTURE_COMPRESSION
#ifdef HAS_XBOX_D3D
    if (!m_linearTexture)
      p3DDevice->SetPalette( 0, m_pPalette);
#endif
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
    p3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    p3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    p3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    p3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
#ifdef HAS_XBOX_D3D
    p3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE);
#endif

#define MIX_ALPHA(a,c) (((a * (c >> 24)) / 255) << 24) | (c & 0x00ffffff)

    // set the base colour based on our alpha level and diffuse color
    D3DCOLOR baseColor = m_colDiffuse;
    if (m_dwAlpha != 0xFF) baseColor = MIX_ALPHA(m_dwAlpha, m_colDiffuse);

    p3DDevice->SetVertexShader( FVF_VERTEX );
#ifdef HAS_XBOX_D3D
    p3DDevice->Begin(D3DPT_QUADLIST);
#else
    p3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
#endif

    float uLeft, uRight, vTop, vBottom;

#ifdef ALLOW_TEXTURE_COMPRESSION
    if (!m_linearTexture)
    {
      uLeft = m_image.border.left / m_iTextureWidth;
      uRight = m_fU - m_image.border.right / m_iTextureWidth;
      vTop = m_image.border.top / m_iTextureHeight;
      vBottom = m_fV - m_image.border.bottom / m_iTextureHeight;
    }
    else
    {
#endif
      uLeft = m_image.border.left;
      uRight = m_fU - m_image.border.right;
      vTop = m_image.border.top;
      vBottom = m_fV - m_image.border.bottom;
#ifdef ALLOW_TEXTURE_COMPRESSION
    }
#endif
    // left segment
    if (m_image.border.left)
    {
      if (m_image.border.top)
        Render(m_fX, m_fY, m_fX + m_image.border.left, m_fY + m_image.border.top, 0, 0, uLeft, vTop, baseColor);
      Render(m_fX, m_fY + m_image.border.top, m_fX + m_image.border.left, m_fY + m_fNH - m_image.border.bottom, 0, vTop, uLeft, vBottom, baseColor);
      if (m_image.border.bottom)
        Render(m_fX, m_fY + m_fNH - m_image.border.bottom, m_fX + m_image.border.left, m_fY + m_fNH, 0, vBottom, uLeft, m_fV, baseColor); 
    }
    // middle segment
    if (m_image.border.top)
      Render(m_fX + m_image.border.left, m_fY, m_fX + m_fNW - m_image.border.right, m_fY + m_image.border.top, uLeft, 0, uRight, vTop, baseColor);
    Render(m_fX + m_image.border.left, m_fY + m_image.border.top, m_fX + m_fNW - m_image.border.right, m_fY + m_fNH - m_image.border.bottom, uLeft, vTop, uRight, vBottom, baseColor);
    if (m_image.border.bottom)
      Render(m_fX + m_image.border.left, m_fY + m_fNH - m_image.border.bottom, m_fX + m_fNW - m_image.border.right, m_fY + m_fNH, uLeft, vBottom, uRight, m_fV, baseColor); 
    // right segment
    if (m_image.border.right)
    { // have a left border
      if (m_image.border.top)
        Render(m_fX + m_fNW - m_image.border.right, m_fY, m_fX + m_fNW, m_fY + m_image.border.top, uRight, 0, m_fU, vTop, baseColor);
      Render(m_fX + m_fNW - m_image.border.right, m_fY + m_image.border.top, m_fX + m_fNW, m_fY + m_fNH - m_image.border.bottom, uRight, vTop, m_fU, vBottom, baseColor);
      if (m_image.border.bottom)
        Render(m_fX + m_fNW - m_image.border.right, m_fY + m_fNH - m_image.border.bottom, m_fX + m_fNW, m_fY + m_fNH, uRight, vBottom, m_fU, m_fV, baseColor); 
    } 
#ifdef ALLOW_TEXTURE_COMPRESSION
#ifdef HAS_XBOX_D3D
    p3DDevice->End();
    if (!m_linearTexture)
      p3DDevice->SetPalette( 0, NULL);
#endif
#endif
    // unset the texture and palette or the texture caching crashes because the runtime still has a reference
    p3DDevice->SetTexture( 0, NULL);
    if (m_fNW > m_width || m_fNH > m_height)
      g_graphicsContext.RestoreViewPort();
  }
  CGUIControl::Render();
}

void CGUIImage::Render(float left, float top, float right, float bottom, float u1, float v1, float u2, float v2, DWORD baseColor)
{
  LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();

  float x1 = floor(g_graphicsContext.ScaleFinalXCoord(left, top) + 0.5f) - 0.5f;
  float y1 = floor(g_graphicsContext.ScaleFinalYCoord(left, top) + 0.5f) - 0.5f;
  float x2 = floor(g_graphicsContext.ScaleFinalXCoord(right, top) + 0.5f) - 0.5f;
  float y2 = floor(g_graphicsContext.ScaleFinalYCoord(right, top) + 0.5f) - 0.5f;
  float x3 = floor(g_graphicsContext.ScaleFinalXCoord(right, bottom) + 0.5f) - 0.5f;
  float y3 = floor(g_graphicsContext.ScaleFinalYCoord(right, bottom) + 0.5f) - 0.5f;
  float x4 = floor(g_graphicsContext.ScaleFinalXCoord(left, bottom) + 0.5f) - 0.5f;
  float y4 = floor(g_graphicsContext.ScaleFinalYCoord(left, bottom) + 0.5f) - 0.5f;

  if (y3 == y1) y3 += 1.0f; if (x2 == x1) x2 += 1.0f;
  if (y4 == y2) y4 += 1.0f; if (x3 == x4) x3 += 1.0f;

  // Render the image

#ifdef HAS_XBOX_D3D
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, u1, v1);
  D3DCOLOR color = baseColor;
  if (m_cornerAlpha[0] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[0],baseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x1, y1, 0, 0 );

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, u2, v1);
  color = baseColor;
  if (m_cornerAlpha[1] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[1],baseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x2, y2, 0, 0 );

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, u2, v2);
  color = baseColor;
  if (m_cornerAlpha[2] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[2],baseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x3, y3, 0, 0 );

  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, u1, v2);
  color = baseColor;
  if (m_cornerAlpha[3] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[3],baseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x4, y4, 0, 0 );

#else
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      FLOAT rhw;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
  };

  CUSTOMVERTEX verts[4];
  verts[0].x = x1; verts[0].y = y1; verts[0].z = 0.0f; verts[0].rhw = 1.0f;
  verts[0].tu = u1;   verts[0].tv = v1;
  DWORD color = baseColor;
  if (m_cornerAlpha[0] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[0],baseColor);
  verts[0].color = g_graphicsContext.MergeAlpha(color);

  verts[1].x = x2; verts[1].y = y2; verts[1].z = 0.0f;verts[1].rhw = 1.0f;
  verts[1].tu = u2;   verts[1].tv = v1;
  color = baseColor;
  if (m_cornerAlpha[1] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[1],baseColor);
  verts[1].color = g_graphicsContext.MergeAlpha(color);

  verts[2].x = x3; verts[2].y = y3; verts[2].z = 0.0f; verts[2].rhw = 1.0f;
  verts[2].tu = u2;   verts[2].tv = v2;
  color = baseColor;
  if (m_cornerAlpha[2] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[2],baseColor);
  verts[2].color = g_graphicsContext.MergeAlpha(color);

  verts[3].x = x4; verts[3].y = y4; verts[3].z = 0.0f; verts[3].rhw = 1.0f;
  verts[3].tu = u1;   verts[3].tv = v2;
  color = baseColor;
  if (m_cornerAlpha[3] != 0xFF) color = MIX_ALPHA(m_cornerAlpha[3],baseColor);
  verts[3].color = g_graphicsContext.MergeAlpha(color);

  p3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
#endif
}

bool CGUIImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIImage::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (m_Info)
      FreeResources();
    return true;
  }
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
  FreeTextures();
  CGUIControl::AllocResources();

  m_dwFrameCounter = 0;
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;

  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey);
  // set allocated to true even if we couldn't load the image to save
  // use hitting the disk every frame
  m_texturesAllocated = true;
  if (!iImages) return ;
  for (int i = 0; i < iImages; i++)
  {
    LPDIRECT3DTEXTURE8 pTexture;
    pTexture = g_TextureManager.GetTexture(m_strFileName, i, m_iTextureWidth, m_iTextureHeight, m_pPalette, m_linearTexture);
#ifndef HAS_XBOX_D3D
    m_linearTexture = false;
#endif
    m_vecTextures.push_back(pTexture);
  }

  // Set state to render the image
  CalculateSize();
}

void CGUIImage::FreeTextures()
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
  m_texturesAllocated = false;
}

void CGUIImage::FreeResources()
{
  FreeTextures();
  CGUIControl::FreeResources();
}

void CGUIImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

void CGUIImage::CalculateSize()
{
  if (m_vecTextures.size() == 0) return ;

  m_fX = m_posX;
  m_fY = m_posY;
  if (!m_linearTexture)
  {
    if (0 == m_iImageWidth || 0 == m_iImageHeight)
    {
      D3DSURFACE_DESC desc;
      m_vecTextures[m_iCurrentImage]->GetLevelDesc(0, &desc);

      m_iImageWidth = desc.Width;
      m_iImageHeight = desc.Height;
    }

    if (0 == m_iTextureWidth || 0 == m_iTextureHeight)
    {
      m_iTextureWidth = m_iImageWidth;
      m_iTextureHeight = m_iImageHeight;

      if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

      if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
    }
  }
  else
  {
    if (0 == m_iTextureWidth || 0 == m_iTextureHeight)
    {
      D3DSURFACE_DESC desc;
      m_vecTextures[m_iCurrentImage]->GetLevelDesc(0, &desc);

      m_iTextureWidth = desc.Width;
      m_iTextureHeight = desc.Height;

      if (m_iTextureHeight > g_graphicsContext.GetHeight() )
        m_iTextureHeight = g_graphicsContext.GetHeight();

      if (m_iTextureWidth > g_graphicsContext.GetWidth() )
        m_iTextureWidth = g_graphicsContext.GetWidth();
    }
  }

  if (m_width == 0)
    m_width = (float)m_iTextureWidth;
  if (m_height == 0)
    m_height = (float)m_iTextureHeight;


  m_fNW = m_width;
  m_fNH = m_height;

  if (m_aspectRatio != ASPECT_RATIO_STRETCH && m_iTextureWidth && m_iTextureHeight)
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = g_graphicsContext.GetScalingPixelRatio();

    float fSourceFrameRatio = ((float)m_iTextureWidth) / ((float)m_iTextureHeight);
    float fOutputFrameRatio = fSourceFrameRatio / pixelRatio;

    // maximize the thumbnails width
    float fNewWidth = fabs(m_width);
    float fNewHeight = fNewWidth / fOutputFrameRatio;

    if ((m_aspectRatio == CGUIImage::ASPECT_RATIO_SCALE && fNewHeight < fabs(m_height)) ||
        (m_aspectRatio == CGUIImage::ASPECT_RATIO_KEEP && fNewHeight > fabs(m_height)))
    {
      fNewHeight = fabs(m_height);
      fNewWidth = fNewHeight * fOutputFrameRatio;
    }
    if (m_aspectRatio == CGUIImage::ASPECT_RATIO_CENTER)
    { // keep original size + center
      fNewWidth = (float)m_iTextureWidth;
      fNewHeight = (float)m_iTextureHeight;
    }
    m_fNW = (m_width < 0) ? -fNewWidth : fNewWidth;
    m_fNH = (m_height < 0) ? -fNewHeight : fNewHeight;
    m_fX = m_posX - (m_fNW - m_width) * 0.5f;
    m_fY = m_posY - (m_fNH - m_height) * 0.5f;
  }

  m_renderWidth = fabs(m_fNW);
  if (fabs(m_width) < m_renderWidth) m_renderWidth = m_width;
  m_renderHeight = fabs(m_fNH);
  if (fabs(m_height) < m_renderHeight) m_renderHeight = m_height;

  if (!m_linearTexture)
  {
    m_fU = float(m_iTextureWidth) / float(m_iImageWidth);
    m_fV = float(m_iTextureHeight) / float(m_iImageHeight);
  }
  else
  {
    m_fU = float(m_iTextureWidth);
    m_fV = float(m_iTextureHeight);
  }
}

bool CGUIImage::CanFocus() const
{
  return false;
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
    Update();
  }
}

float CGUIImage::GetRenderWidth() const
{
  return m_renderWidth;
}

float CGUIImage::GetRenderHeight() const
{
  return m_renderHeight;
}

void CGUIImage::PythonSetColorKey(DWORD dwColorKey)
{
  m_dwColorKey = dwColorKey;
}

void CGUIImage::SetFileName(const CStdString& strFileName)
{
  if (strFileName.IsEmpty() && !m_image.file.IsEmpty())
    return SetFileName(m_image.file);

  if (m_strFileName.Equals(strFileName)) return;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeTextures();
  m_strFileName = strFileName;
  // Don't allocate resources here as this is done at render time
}

void CGUIImage::SetCornerAlpha(DWORD dwLeftTop, DWORD dwRightTop, DWORD dwLeftBottom, DWORD dwRightBottom)
{
  if (
    m_cornerAlpha[0] != dwLeftTop ||
    m_cornerAlpha[1] != dwRightTop ||
    m_cornerAlpha[2] != dwLeftBottom ||
    m_cornerAlpha[3] != dwRightBottom
  )
  {
    m_cornerAlpha[0] = dwLeftTop;
    m_cornerAlpha[1] = dwRightTop;
    m_cornerAlpha[2] = dwLeftBottom;
    m_cornerAlpha[3] = dwRightBottom;
    Update();
  }
}

void CGUIImage::SetAlpha(DWORD dwAlpha)
{
  m_dwAlpha = dwAlpha;
}

void CGUIImage::GetBottomRight(float &x, float &y) const
{
  x = m_fX + m_fNW;
  y = m_fY + m_fNH;
  if (m_fNW > m_width)
    x = m_posX + m_width;
  if (m_fNH > m_height)
    y = m_posY + m_height;
}

bool CGUIImage::IsAllocated() const
{
  if (!m_texturesAllocated) return false;
  return CGUIControl::IsAllocated();
}
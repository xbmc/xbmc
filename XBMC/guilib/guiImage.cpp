#include "include.h"
#include "guiImage.h"
#include "TextureManager.h"
#include "../xbmc/Settings.h"
#include "../xbmc/utils/GUIInfoManager.h"

#define MIX_ALPHA(a,c) (((a * (c >> 24)) / 255) << 24) | (c & 0x00ffffff)

CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, DWORD dwColorKey)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  memset(m_alpha, 0xff, 4);
  m_image = texture;
  g_infoManager.ParseLabel(m_image.file, m_multiInfo);
  if (m_multiInfo.size() == 1 && !m_multiInfo[0].m_info)
    m_multiInfo.clear();  // no info here at all - just a standard texture
  if (m_multiInfo.size())
    m_image.file.Empty(); // have multiinfo, so no fallback texture
  m_strFileName = m_image.file;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  m_dwColorKey = dwColorKey;
  m_iCurrentImage = 0;
  m_dwFrameCounter = (DWORD) -1;
  m_aspectRatio = ASPECT_RATIO_STRETCH;
  m_aspectAlign = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
  m_singleInfo = 0;
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;
}

CGUIImage::CGUIImage(const CGUIImage &left)
    : CGUIControl(left)
{
  m_strFileName = left.m_strFileName;
  m_dwColorKey = left.m_dwColorKey;
  m_aspectRatio = left.m_aspectRatio;
  m_aspectAlign = left.m_aspectAlign;
  // defaults
  m_iCurrentImage = 0;
  m_dwFrameCounter = (DWORD) -1;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  memcpy(m_alpha, left.m_alpha, 4);
  m_pPalette = NULL;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
  m_multiInfo = left.m_multiInfo;
  m_singleInfo = left.m_singleInfo;
  m_image = left.m_image;
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;
}

CGUIImage::~CGUIImage(void)
{

}

void CGUIImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // check for conditional information before we free and
  // alloc as this does free and allocation as well
  if (m_multiInfo.size())
  {
    SetFileName(g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID, true));
  }
  if (m_singleInfo)
  {
    SetFileName(g_infoManager.GetImage(m_singleInfo, m_dwParentID));
  }

  AllocateOnDemand();
}

void CGUIImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResources();
    // reset animated textures (animgifs)
    m_iCurrentLoop = 0;
    m_iCurrentImage = 0;
    m_dwFrameCounter = 0;
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_texturesAllocated)
    AllocResources();
}

void CGUIImage::Render()
{
  // we need the checks for visibility and resource allocation here as
  // most controls use CGUIImage's to do their rendering (where UpdateVisibility doesn't apply)
  AllocateOnDemand();

  if (!IsVisible()) return;

  if (m_vecTextures.size())
  {
    Process();
    if (m_bInvalidated) CalculateSize();
    // scale to screen output position
    if (m_fNW > m_width || m_fNH > m_height)
    {
      if (!g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
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
    if (m_diffusePalette)
      p3DDevice->SetPalette( 1, m_diffusePalette);
#endif
#endif
    p3DDevice->SetTexture( 0, m_vecTextures[m_iCurrentImage] );
    p3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
    p3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    if (m_diffuseTexture)
    {
      p3DDevice->SetTexture( 1, m_diffuseTexture );
      p3DDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
      p3DDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
      p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
      p3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    }

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
    p3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

#ifdef HAS_XBOX_D3D
    p3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE);
#endif

    p3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 );

    float uLeft, uRight, vTop, vBottom;

#ifdef ALLOW_TEXTURE_COMPRESSION
    if (!m_linearTexture)
    {
      uLeft = m_image.border.left / m_iImageWidth;
      uRight = m_fU - m_image.border.right / m_iImageWidth;
      vTop = m_image.border.top / m_iImageHeight;
      vBottom = m_fV - m_image.border.bottom / m_iImageHeight;
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

#ifdef HAS_XBOX_D3D
    p3DDevice->Begin(D3DPT_QUADLIST);
#endif

    // TODO: The diffuse coloring applies to all vertices, which will
    //       look weird for stuff with borders, as will the -ve height/width
    //       for flipping

    // left segment
    if (m_image.border.left)
    {
      if (m_image.border.top)
        Render(m_fX, m_fY, m_fX + m_image.border.left, m_fY + m_image.border.top, 0, 0, uLeft, vTop);
      Render(m_fX, m_fY + m_image.border.top, m_fX + m_image.border.left, m_fY + m_fNH - m_image.border.bottom, 0, vTop, uLeft, vBottom);
      if (m_image.border.bottom)
        Render(m_fX, m_fY + m_fNH - m_image.border.bottom, m_fX + m_image.border.left, m_fY + m_fNH, 0, vBottom, uLeft, m_fV); 
    }
    // middle segment
    if (m_image.border.top)
      Render(m_fX + m_image.border.left, m_fY, m_fX + m_fNW - m_image.border.right, m_fY + m_image.border.top, uLeft, 0, uRight, vTop);
    Render(m_fX + m_image.border.left, m_fY + m_image.border.top, m_fX + m_fNW - m_image.border.right, m_fY + m_fNH - m_image.border.bottom, uLeft, vTop, uRight, vBottom);
    if (m_image.border.bottom)
      Render(m_fX + m_image.border.left, m_fY + m_fNH - m_image.border.bottom, m_fX + m_fNW - m_image.border.right, m_fY + m_fNH, uLeft, vBottom, uRight, m_fV); 
    // right segment
    if (m_image.border.right)
    { // have a left border
      if (m_image.border.top)
        Render(m_fX + m_fNW - m_image.border.right, m_fY, m_fX + m_fNW, m_fY + m_image.border.top, uRight, 0, m_fU, vTop);
      Render(m_fX + m_fNW - m_image.border.right, m_fY + m_image.border.top, m_fX + m_fNW, m_fY + m_fNH - m_image.border.bottom, uRight, vTop, m_fU, vBottom);
      if (m_image.border.bottom)
        Render(m_fX + m_fNW - m_image.border.right, m_fY + m_fNH - m_image.border.bottom, m_fX + m_fNW, m_fY + m_fNH, uRight, vBottom, m_fU, m_fV); 
    } 
#ifdef ALLOW_TEXTURE_COMPRESSION
#ifdef HAS_XBOX_D3D
    p3DDevice->End();
    if (g_graphicsContext.RectIsAngled(m_fX, m_fY, m_fX + m_fNW, m_fY + m_fNH))
    {
      p3DDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE );
      p3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS, TRUE );
      p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
      p3DDevice->Begin(D3DPT_QUADLIST);
      Render(m_fX, m_fY, m_fX + m_fNW, m_fY + m_fNH, 0, 0, m_fU, m_fV);
      p3DDevice->End();
      p3DDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
      p3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS, FALSE );
      p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }
    if (!m_linearTexture)
      p3DDevice->SetPalette( 0, NULL);
    if (m_diffusePalette)
      p3DDevice->SetPalette( 1, NULL);
#endif
#endif
    // unset the texture and palette or the texture caching crashes because the runtime still has a reference
    p3DDevice->SetTexture( 0, NULL );
    if (m_diffuseTexture)
      p3DDevice->SetTexture( 1, NULL );
    if (m_fNW > m_width || m_fNH > m_height)
      g_graphicsContext.RestoreClipRegion();
  }
  CGUIControl::Render();
}

void CGUIImage::Render(float left, float top, float right, float bottom, float u1, float v1, float u2, float v2)
{
  LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();

  // flip the texture as necessary.  Diffuse just gets flipped according to m_image.orientation.
  // Main texture gets flipped according to GetOrientation().
  CRect diffuse(u1, v1, u2, v2);
  OrientateTexture(diffuse, m_image.orientation);
  CRect texture(u1, v1, u2, v2);
  int textureOrientation = GetOrientation();
  OrientateTexture(texture, textureOrientation);

  CRect vertex(left, top, right, bottom);
  g_graphicsContext.ClipRect(vertex, texture, m_diffuseTexture ? &diffuse : NULL);

  if (vertex.IsEmpty())
    return; // nothing to render

  float x1 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1) + 0.5f) - 0.5f;
  float y1 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1) + 0.5f) - 0.5f;
  float x2 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1) + 0.5f) - 0.5f;
  float y2 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1) + 0.5f) - 0.5f;
  float x3 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2) + 0.5f) - 0.5f;
  float y3 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2) + 0.5f) - 0.5f;
  float x4 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2) + 0.5f) - 0.5f;
  float y4 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2) + 0.5f) - 0.5f;
  float z1 = g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y1);
  float z2 = g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y1);
  float z3 = g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y2);
  float z4 = g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y2);

  if (y3 == y1) y3 += 1.0f; if (x3 == x1) x3 += 1.0f;
  if (y4 == y2) y4 += 1.0f; if (x4 == x2) x4 += 1.0f;

  diffuse.x1 *= m_diffuseScaleU; diffuse.x2 *= m_diffuseScaleU;
  diffuse.y1 *= m_diffuseScaleV; diffuse.y2 *= m_diffuseScaleV;

#ifdef HAS_XBOX_D3D
  D3DCOLOR color = m_diffuseColor;
  if (m_alpha[0] != 0xFF) color = MIX_ALPHA(m_alpha[0],m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y1);
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x1, diffuse.y1);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x1, y1, z1, 1 );

  color = m_diffuseColor;
  if (m_alpha[1] != 0xFF) color = MIX_ALPHA(m_alpha[1],m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  if (textureOrientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y2);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y1);
  if (m_image.orientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x1, diffuse.y2);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x2, diffuse.y1);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x2, y2, z2, 1 );

  color =  m_diffuseColor;
  if (m_alpha[2] != 0xFF) color = MIX_ALPHA(m_alpha[2], m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y2);
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x2, diffuse.y2);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x3, y3, z3, 1 );

  color =  m_diffuseColor;
  if (m_alpha[3] != 0xFF) color = MIX_ALPHA(m_alpha[3], m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  if (textureOrientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y1);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y2);
  if (m_image.orientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x2, diffuse.y1);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x1, diffuse.y2);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x4, y4, z4, 1 );
#else
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  CUSTOMVERTEX verts[4];
  verts[0].x = x1; verts[0].y = y1; verts[0].z = z1;
  verts[0].tu = texture.x1;   verts[0].tv = texture.y1;
  verts[0].tu2 = diffuse.x1;  verts[0].tv2 = diffuse.y1;
  DWORD color = m_diffuseColor;
  if (m_alpha[0] != 0xFF) color = MIX_ALPHA(m_alpha[0],m_diffuseColor);
  verts[0].color = g_graphicsContext.MergeAlpha(color);

  verts[1].x = x2; verts[1].y = y2; verts[1].z = z2;
  if (textureOrientation & 4)
  {
    verts[1].tu = texture.x1;
    verts[1].tv = texture.y2;
  }
  else
  {
    verts[1].tu = texture.x2;
    verts[1].tv = texture.y1;
  }
  if (m_image.orientation & 4)
  {
    verts[1].tu2 = diffuse.x1;
    verts[1].tv2 = diffuse.y2;
  }
  else
  {
    verts[1].tu2 = diffuse.x2;
    verts[1].tv2 = diffuse.y1;
  }

  color = m_diffuseColor;
  if (m_alpha[1] != 0xFF) color = MIX_ALPHA(m_alpha[1],m_diffuseColor);
  verts[1].color = g_graphicsContext.MergeAlpha(color);

  verts[2].x = x3; verts[2].y = y3; verts[2].z = z3;
  verts[2].tu = texture.x2;   verts[2].tv = texture.y2;
  verts[2].tu2 = diffuse.x2;  verts[2].tv2 = diffuse.y2;
  color = m_diffuseColor;
  if (m_alpha[2] != 0xFF) color = MIX_ALPHA(m_alpha[2],m_diffuseColor);
  verts[2].color = g_graphicsContext.MergeAlpha(color);

  verts[3].x = x4; verts[3].y = y4; verts[3].z = z4;
  if (textureOrientation & 4)
  {
    verts[3].tu = texture.x2;
    verts[3].tv = texture.y1;
  }
  else
  {
    verts[3].tu = texture.x1;
    verts[3].tv = texture.y2;
  }
  if (m_image.orientation & 4)
  {
    verts[3].tu2 = diffuse.x2;
    verts[3].tv2 = diffuse.y1;
  }
  else
  {
    verts[3].tu2 = diffuse.x1;
    verts[3].tv2 = diffuse.y2;
  }
  color = m_diffuseColor;
  if (m_alpha[3] != 0xFF) color = MIX_ALPHA(m_alpha[3],m_diffuseColor);
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
    if (m_singleInfo || m_multiInfo.size())
      FreeResources();
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIImage::PreAllocResources()
{
  FreeResources();
  g_TextureManager.PreLoad(m_strFileName);
  if (!m_image.diffuse.IsEmpty())
    g_TextureManager.PreLoad(m_image.diffuse);
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

  CalculateSize();

  LoadDiffuseImage();
}

void CGUIImage::LoadDiffuseImage()
{
  m_diffuseScaleU = m_diffuseScaleV = 1.0f;
  // load the diffuse texture (if necessary)
  if (!m_image.diffuse.IsEmpty())
  {
    int width, height;
    bool linearTexture;
    int iImages = g_TextureManager.Load(m_image.diffuse, 0);
    m_diffuseTexture = g_TextureManager.GetTexture(m_image.diffuse, 0, width, height, m_diffusePalette, linearTexture);

    if (m_diffuseTexture)
    { // calculate scaling for the texcoords (vs texcoords of main texture)
#ifdef HAS_XBOX_D3D
      if (linearTexture)
      {
        m_diffuseScaleU = float(width) / m_fU;
        m_diffuseScaleV = float(height) / m_fV;
      }
      else
#endif
      {
        D3DSURFACE_DESC desc;
        m_diffuseTexture->GetLevelDesc(0, &desc);
        m_diffuseScaleU = float(width) / float(desc.Width) / m_fU;
        m_diffuseScaleV = float(height) / float(desc.Height) / m_fV;
      }
    }
  }
}

void CGUIImage::FreeTextures()
{
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    g_TextureManager.ReleaseTexture(m_strFileName, i);
  }

  if (m_diffuseTexture)
    g_TextureManager.ReleaseTexture(m_image.diffuse);
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;

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
    if (GetOrientation() & 4)
      fSourceFrameRatio = ((float)m_iTextureHeight) / ((float)m_iTextureWidth);
    float fOutputFrameRatio = fSourceFrameRatio / pixelRatio;

    // maximize the thumbnails width
    m_fNW = m_width;
    m_fNH = m_fNW / fOutputFrameRatio;

    if ((m_aspectRatio == CGUIImage::ASPECT_RATIO_SCALE && m_fNH < m_height) ||
        (m_aspectRatio == CGUIImage::ASPECT_RATIO_KEEP && m_fNH > m_height))
    {
      m_fNH = m_height;
      m_fNW = m_fNH * fOutputFrameRatio;
    }
    if (m_aspectRatio == CGUIImage::ASPECT_RATIO_CENTER)
    { // keep original size + center
      m_fNW = (float)m_iTextureWidth;
      m_fNH = (float)m_iTextureHeight;
    }

    // calculate placement
    if (m_aspectAlign & ASPECT_ALIGN_LEFT)
      m_fX = m_posX;
    else if (m_aspectAlign & ASPECT_ALIGN_RIGHT)
      m_fX = m_posX + m_width - m_fNW;
    else
      m_fX = m_posX + (m_width - m_fNW) * 0.5f;
    if (m_aspectAlign & ASPECT_ALIGNY_TOP)
      m_fY = m_posY;
    else if (m_aspectAlign & ASPECT_ALIGNY_BOTTOM)
      m_fY = m_posY + m_height - m_fNH;
    else
      m_fY = m_posY + (m_height - m_fNH) * 0.5f;
  }

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

void CGUIImage::SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio, DWORD align)
{
  if (m_aspectRatio != ratio || m_aspectAlign != align)
  {
    m_aspectRatio = ratio;
    m_aspectAlign = align;
    Update();
  }
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

void CGUIImage::SetAlpha(unsigned char alpha)
{
  SetAlpha(alpha, alpha, alpha, alpha);
}

void CGUIImage::SetAlpha(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3)
{
  m_alpha[0] = a0;
  m_alpha[1] = a1;
  m_alpha[2] = a2;
  m_alpha[3] = a3;
}

void CGUIImage::SetInfo(int info)
{
  m_singleInfo = info;
}

bool CGUIImage::IsAllocated() const
{
  if (!m_texturesAllocated) return false;
  return CGUIControl::IsAllocated();
}

#ifdef _DEBUG
void CGUIImage::DumpTextureUse()
{
  if (m_texturesAllocated && m_vecTextures.size())
  {
    if (GetID())
      CLog::Log(LOGDEBUG, "Image control %d using texture %s", GetID(), m_strFileName.c_str());
    else
      CLog::Log(LOGDEBUG, "Using texture %s", m_strFileName.c_str());
  }
}
#endif

void CGUIImage::OrientateTexture(CRect &rect, int orientation)
{
  switch (orientation & 3)
  {
  case 0:
    // default
    break;
  case 1:
    // flip in X direction
    rect.x1 = m_fU - rect.x1;
    rect.x2 = m_fU - rect.x2;
    break;
  case 2:
    // rotate 180 degrees
    rect.x1 = m_fU - rect.x1;
    rect.x2 = m_fU - rect.x2;
    rect.y1 = m_fV - rect.y1;
    rect.y2 = m_fV - rect.y2;
    break;
  case 3:
    // flip in Y direction
    rect.y1 = m_fV - rect.y1;
    rect.y2 = m_fV - rect.y2;
    break;
  }
  if (orientation & 4)
  {
    swap(rect.x1, rect.y1);
    swap(rect.x2, rect.y2);
  }
}

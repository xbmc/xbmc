#include "guiimage.h"
#include "texturemanager.h"



CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture,DWORD dwColorKey)
:CGUIControl(dwParentID, dwControlId,dwPosX, dwPosY, dwWidth, dwHeight)
{
  m_colDiffuse	= 0xFFFFFFFF;  
  m_pTexture=NULL;
  m_pVB=NULL;
  m_strFileName=strTexture;
  m_iTextureWidth=0;
  m_iTextureHeight=0;
  m_dwColorKey=dwColorKey;
  m_iBitmap=0;
  m_dwItems=1;
}


CGUIImage::~CGUIImage(void)
{
}
void CGUIImage::Render()
{
  if (!IsVisible()) return;
	if (!m_pTexture)
		return ;
	if (!m_pVB)
		return ;

    // Set state to render the image
    g_graphicsContext.Get3DDevice()->SetTexture( 0, m_pTexture );
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
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
    // Render the image
    g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

}

void CGUIImage::OnKey(const CKey& key) 
{
}

bool CGUIImage::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIImage::AllocResources()
{
  FreeResources();

  // Create a vertex buffer for rendering the image
  g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(CGUIImage::VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVB );

  // load texture...
  m_pTexture=g_TextureManager.GetTexture(m_strFileName,m_dwColorKey);
  if (!m_pTexture) return;

  // Set state to render the image
  Update();
}

void CGUIImage::FreeResources()
{
  if (m_pVB!=NULL)
	{
		m_pVB->Release();
		m_pVB=NULL;
	}

  if (m_pTexture)
	{
	  m_pTexture->Release();
		m_pTexture= NULL;
	}
}


void CGUIImage::Update()
{
  if (!m_pVB) return;
  if (!m_pTexture) return;

  CGUIImage::VERTEX* vertex=NULL;
  m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

  float x=(float)m_dwPosX;
  float y=(float)m_dwPosY;
  
  if (0==m_iTextureWidth|| 0==m_iTextureHeight)
  {
    D3DSURFACE_DESC desc;
	  m_pTexture->GetLevelDesc(0,&desc);

	  m_iTextureWidth  = (DWORD) desc.Width/m_dwItems;
	  m_iTextureHeight = (DWORD) desc.Height;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
  if (m_dwWidth && m_dwItems>1)
  {
    m_iTextureWidth=m_dwWidth;
  }


  if (m_dwWidth==0) 
    m_dwWidth=m_iTextureWidth;
  if (m_dwHeight==0) 
    m_dwHeight=m_iTextureHeight;

  float nw =(float)m_dwWidth;
  float nh=(float)m_dwHeight;

  int iXOffset=m_iBitmap*m_dwWidth;

  vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
  vertex[0].tu = (float)iXOffset;
  vertex[0].tv = 0;

  vertex[1].p = D3DXVECTOR4( x+nw - 0.5f,	y - 0.5f,		0, 0 );
  vertex[1].tu = (float)iXOffset+m_iTextureWidth;
  vertex[1].tv = 0;

  vertex[2].p = D3DXVECTOR4( x+nw - 0.5f,	y+nh - 0.5f,	0, 0 );
  vertex[2].tu = (float)iXOffset+m_iTextureWidth;
  vertex[2].tv = (float)m_iTextureHeight;

  vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+nh - 0.5f,	0, 0 );
  vertex[3].tu = (float)iXOffset;
  vertex[3].tv = (float)m_iTextureHeight;
 
  vertex[0].col = m_colDiffuse;
	vertex[1].col = m_colDiffuse;
	vertex[2].col = m_colDiffuse;
	vertex[3].col = m_colDiffuse;
  m_pVB->Unlock();  
}

bool CGUIImage::CanFocus() const
{
  return false;
}

void CGUIImage::Select(int iBitmap)
{
  m_iBitmap=iBitmap;
  Update();
}

void CGUIImage::SetItems(int iItems)
{
  m_dwItems=iItems;
}
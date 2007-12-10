#include "include.h"
#include "GUIBorderedImage.h"

CGUIBorderedImage::CGUIBorderedImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, const CImage& borderTexture, const FRECT &borderSize, DWORD dwColorKey)
   : CGUIImage(dwParentID, dwControlId, posX + borderSize.left, posY + borderSize.top, width - borderSize.left - borderSize.right, height - borderSize.top - borderSize.bottom, texture, dwColorKey),
     m_borderImage(dwParentID, dwControlId, posX, posY, width, height, borderTexture, dwColorKey)
{
  memcpy(&m_borderSize, &borderSize, sizeof(FRECT));
}

CGUIBorderedImage::~CGUIBorderedImage(void)
{
}

void CGUIBorderedImage::Render()
{
  if (!m_borderImage.GetFileName().IsEmpty() && m_vecTextures.size())
  {
    if (m_bInvalidated) CGUIImage::CalculateSize();

    m_borderImage.SetPosition(m_fX - m_borderSize.left, m_fY - m_borderSize.top);
    m_borderImage.SetWidth(m_fNW + m_borderSize.left + m_borderSize.right);
    m_borderImage.SetHeight(m_fNH + m_borderSize.top + m_borderSize.bottom);

    m_borderImage.Render();
  }
  CGUIImage::Render();
}

void CGUIBorderedImage::UpdateVisibility()
{
  m_borderImage.UpdateVisibility();
  CGUIImage::UpdateVisibility();
}

bool CGUIBorderedImage::OnMessage(CGUIMessage& message)
{
  return m_borderImage.OnMessage(message) && CGUIImage::OnMessage(message);
}

void CGUIBorderedImage::PreAllocResources()
{
  m_borderImage.PreAllocResources();
  CGUIImage::PreAllocResources();
}

void CGUIBorderedImage::AllocResources()
{
  m_borderImage.AllocResources();
  CGUIImage::AllocResources();
}

void CGUIBorderedImage::FreeResources()
{
  m_borderImage.FreeResources();
  CGUIImage::FreeResources();
}

void CGUIBorderedImage::DynamicResourceAlloc(bool bOnOff)
{
  m_borderImage.DynamicResourceAlloc(bOnOff);
  CGUIImage::DynamicResourceAlloc(bOnOff);
}

bool CGUIBorderedImage::IsAllocated() const
{
  return m_borderImage.IsAllocated() && CGUIImage::IsAllocated();
}

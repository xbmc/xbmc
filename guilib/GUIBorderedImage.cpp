#include "include.h"
#include "GUIBorderedImage.h"

CGUIBorderedImage::CGUIBorderedImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, const CImage& borderTexture, float borderSize, DWORD dwColorKey)
   : CGUIImage(dwParentID, dwControlId, posX + borderSize, posY + borderSize, width - borderSize - borderSize, height - borderSize - borderSize, texture, dwColorKey),
     m_borderImage(dwParentID, dwControlId, posX, posY, width, height, borderTexture, dwColorKey)
{
}

CGUIBorderedImage::~CGUIBorderedImage(void)
{
}

void CGUIBorderedImage::Render()
{
   m_borderImage.Render();
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

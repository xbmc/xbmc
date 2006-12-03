#include "include.h"
#include "GUIListItemLayout.h"
#include "GUIListItem.h"
#include "GUIControlFactory.h"
#include "GUIFontManager.h"
#include "XMLUtils.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/FileItem.h"

CGUIListItemLayout::CListBase::CListBase(float posX, float posY, float width, float height)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
}

CGUIListItemLayout::CListBase::~CListBase()
{
}

CGUIListItemLayout::CListLabel::CListLabel(float posX, float posY, float width, float height, const CLabelInfo &label, int info, const CStdString &content)
: CGUIListItemLayout::CListBase(posX, posY, width, height)
{
  m_label = label;
  m_info = info;
  m_type = LIST_LABEL;
  g_infoManager.ParseLabel(content, m_multiInfo);
}

CGUIListItemLayout::CListLabel::~CListLabel()
{
}

CGUIListItemLayout::CListTexture::CListTexture(float posX, float posY, float width, float height, const CImage &image, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio)
: CGUIListItemLayout::CListBase(posX, posY, width, height),
  m_image(0, 0, posX, posY, width, height, image)
{
  m_type = LIST_TEXTURE;
  m_image.SetAspectRatio(aspectRatio);
}

CGUIListItemLayout::CListTexture::~CListTexture()
{
  m_image.FreeResources();
}

CGUIListItemLayout::CListImage::CListImage(float posX, float posY, float width, float height, const CImage &image, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, int info)
: CGUIListItemLayout::CListTexture(posX, posY, width, height, image, aspectRatio)
{
  m_info = info;
  m_type = LIST_IMAGE;
}

CGUIListItemLayout::CListImage::~CListImage()
{
}

CGUIListItemLayout::CGUIListItemLayout()
{
  m_width = 0;
  m_height = 0;
  m_focused = false;
  m_invalidated = true;
  m_isPlaying = false;
}

CGUIListItemLayout::CGUIListItemLayout(const CGUIListItemLayout &from)
{
  m_width = from.m_width;
  m_height = from.m_height;
  m_focused = from.m_focused;
  // copy across our controls
  for (ciControls it = from.m_controls.begin(); it != from.m_controls.end(); ++it)
  {
    CListBase *item = *it;
    if (item->m_type == CListBase::LIST_LABEL)
      m_controls.push_back(new CListLabel(*(CListLabel *)item));
    else if (item->m_type ==  CListBase::LIST_IMAGE)
      m_controls.push_back(new CListImage(*(CListImage *)item));
    else if (item->m_type ==  CListBase::LIST_TEXTURE)
      m_controls.push_back(new CListTexture(*(CListTexture *)item));
  }
  m_invalidated = true;
  m_isPlaying = false;
}

CGUIListItemLayout::~CGUIListItemLayout()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); ++it)
    delete *it;
}

float CGUIListItemLayout::Size(ORIENTATION orientation)
{
  return (orientation == HORIZONTAL) ? m_width : m_height;
}

void CGUIListItemLayout::Render(CGUIListItem *item)
{
  if (m_invalidated)
  {
    // check for boolean conditions
    m_isPlaying = g_infoManager.GetItemBool((CFileItem *)item, LISTITEM_ISPLAYING);

    for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
      UpdateItem(*it, item);
    // now we have to check our overlapping label pairs
    for (unsigned int i = 0; i < m_controls.size(); i++)
    {
      if (m_controls[i]->m_type == CListBase::LIST_LABEL)
      {
        CListLabel *label1 = (CListLabel *)m_controls[i];
        for (unsigned int j = i + 1; j < m_controls.size(); j++)
        {
          if (m_controls[j]->m_type == CListBase::LIST_LABEL)
          { // ok, now check if they overlap
            CListLabel *label2 = (CListLabel *)m_controls[j];
            if ((label1->m_renderY <= label2->m_renderY + label2->m_renderH*0.5f && label2->m_renderY + label2->m_renderH*0.5f <= label1->m_renderY + label1->m_renderH) ||
                (label2->m_renderY <= label1->m_renderY + label1->m_renderH*0.5f && label1->m_renderY + label1->m_renderH*0.5f <= label2->m_renderY + label2->m_renderH))
            { // overlap vertically - check horizontal
              CListLabel *left = label1->m_renderX < label2->m_renderX ? label1 : label2;
              CListLabel *right = label1->m_renderX < label2->m_renderX ? label2 : label1;
              if ((left->m_label.align & 3) == 0 && right->m_label.align & XBFONT_RIGHT)
              { // left is aligned left, right is aligned right, and they overlap vertically
                if (left->m_renderX + left->m_renderW + 10 > right->m_renderX && left->m_renderX + left->m_renderW < right->m_renderX + right->m_renderW)
                { // overlap, so chop accordingly
                  float chopPoint = (left->m_posX + left->m_width + right->m_posX - right->m_width) * 0.5f;
// [1       [2...[2  1].|..........1]         2]
// [1       [2.....[2   |      1]..1]         2]
// [1       [2..........|.[2   1]..1]         2]
                  if (right->m_renderX > chopPoint)
                    chopPoint = right->m_renderX - 5;
                  else if (left->m_renderX + left->m_renderW < chopPoint)
                    chopPoint = left->m_renderX + left->m_renderW + 5;
                  left->m_renderW = chopPoint - 5 - left->m_renderX;
                  right->m_renderW -= (chopPoint + 5 - right->m_renderX);
                  right->m_renderX = chopPoint + 5;
                }
              }
            }
          }
        }
      }
    }
    m_invalidated = false;
  }

  // and render
  for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
  {
    CListBase *layoutItem = *it;
    if (layoutItem->m_type == CListBase::LIST_LABEL)
      RenderLabel((CListLabel *)layoutItem, item->IsSelected() || m_isPlaying, m_focused);
    else
      ((CListTexture *)layoutItem)->m_image.Render();
  }
}

void CGUIListItemLayout::UpdateItem(CGUIListItemLayout::CListBase *control, CGUIListItem *item)
{
  if (control->m_type == CListBase::LIST_IMAGE)
  {
    CListImage *image = (CListImage *)control;
    image->m_image.SetFileName(g_infoManager.GetItemImage((CFileItem *)item, image->m_info));
  }
  else if (control->m_type == CListBase::LIST_LABEL)
  {
    CListLabel *label = (CListLabel *)control;
    if (label->m_info)
      g_charsetConverter.utf8ToUTF16(g_infoManager.GetItemLabel((CFileItem *)item, label->m_info), label->m_text);
    else
      g_charsetConverter.utf8ToUTF16(g_infoManager.GetItemMultiLabel((CFileItem *)item, label->m_multiInfo), label->m_text);
    if (label->m_label.font)
    {
      label->m_label.font->GetTextExtent(label->m_text, &label->m_textW, &label->m_renderH);
      label->m_renderW = min(label->m_textW, label->m_width);
      if (label->m_label.align & XBFONT_CENTER_Y)
        label->m_renderY = label->m_posY + (label->m_height - label->m_renderH) * 0.5f;
      else
        label->m_renderY = label->m_posY;
      if (label->m_label.align & XBFONT_RIGHT)
        label->m_renderX = label->m_posX - label->m_renderW;
      else if (label->m_label.align & XBFONT_CENTER_X)
        label->m_renderX = label->m_posX - label->m_renderW * 0.5f;
      else
        label->m_renderX = label->m_posX;
    }
  }
}

void CGUIListItemLayout::RenderLabel(CListLabel *label, bool selected, bool scroll)
{
  if (label->m_label.font && !label->m_text.IsEmpty())
  {
    DWORD color = selected ? label->m_label.selectedColor : label->m_label.textColor;
    if (scroll && label->m_renderW < label->m_textW)
      label->m_label.font->DrawScrollingText(label->m_renderX, label->m_renderY, &color, 1,
                                  label->m_label.shadowColor, label->m_text, label->m_renderW, label->m_scrollInfo);
    else
      label->m_label.font->DrawTextWidth(label->m_renderX, label->m_renderY, label->m_label.angle, color,
                                  label->m_label.shadowColor, label->m_text, label->m_renderW);
  }
}

void CGUIListItemLayout::ResetScrolling()
{
  for (iControls it = m_controls.begin(); it != m_controls.end(); it++)
  {
    CListBase *layoutItem = (*it);
    if (layoutItem->m_type == CListBase::LIST_LABEL)
      ((CListLabel *)layoutItem)->m_scrollInfo.Reset();
  }
}


CGUIListItemLayout::CListBase *CGUIListItemLayout::CreateItem(TiXmlElement *child)
{
  // grab the type...
  CGUIControlFactory factory;
  CStdString type = factory.GetType(child);
  CGUIControl *control = factory.Create(0, NULL, child);
  float posX = 0;
  float posY = 0;
  float width = 10;
  float height = 10;
  CStdString infoString;
  CImage image;
  CLabelInfo label;
  XMLUtils::GetFloat(child, "posx", posX);
  XMLUtils::GetFloat(child, "posy", posY);
  XMLUtils::GetFloat(child, "width", width);
  XMLUtils::GetFloat(child, "height", height);
  XMLUtils::GetString(child, "info", infoString);
  XMLUtils::GetHex(child, "textcolor", label.textColor);
  XMLUtils::GetHex(child, "selectedcolor", label.selectedColor);
  XMLUtils::GetHex(child, "shadowcolor", label.shadowColor);
  CStdString fontName;
  XMLUtils::GetString(child, "font", fontName);
  label.font = g_fontManager.GetFont(fontName);
  int info = g_infoManager.TranslateString(infoString);
  if (info && (info < LISTITEM_START || info > LISTITEM_END))
  {
    CLog::Log(LOGERROR, __FUNCTION__" Invalid item info %s", infoString.c_str());
    return NULL;
  }
  factory.GetTexture(child, "texture", image);
  factory.GetAlignment(child, "align", label.align);
  DWORD alignY = 0;
  if (factory.GetAlignmentY(child, "aligny", alignY))
    label.align |= alignY;
  CStdString content;
  XMLUtils::GetString(child, "label", content);
  CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio = CGUIImage::ASPECT_RATIO_KEEP;
  factory.GetAspectRatio(child, "aspectratio", aspectRatio);
  if (type == "label")
  { // info label
    return new CListLabel(posX, posY, width, height, label, info, content);
  }
  else if (type == "image")
  {
    if (info)
    { // info image
      return new CListImage(posX, posY, width, height, image, aspectRatio, info);
    }
    else
    { // texture
      return new CListTexture(posX, posY, width, height, image, CGUIImage::ASPECT_RATIO_STRETCH);
    }
  }
  return NULL;
}

void CGUIListItemLayout::LoadLayout(TiXmlElement *layout, bool focused)
{
  m_focused = focused;
  layout->Attribute("width", &m_width);
  layout->Attribute("height", &m_height);
  TiXmlElement *child = layout->FirstChildElement("control");
  while (child)
  {
    CListBase *item = CreateItem(child);
    if (item)
      m_controls.push_back(item);
    child = child->NextSiblingElement("control");
  }
}

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
void CGUIListItemLayout::CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CImage &texture, float texHeight, float iconWidth, float iconHeight)
{
  m_width = width;
  m_height = height;
  m_focused = focused;
  CListTexture *tex = new CListTexture(0, 0, width, texHeight, texture, CGUIImage::ASPECT_RATIO_STRETCH);
  m_controls.push_back(tex);
  CListImage *image = new CListImage(8, 0, iconWidth, texHeight, CImage(""), CGUIImage::ASPECT_RATIO_KEEP, LISTITEM_ICON);
  m_controls.push_back(image);
  float x = iconWidth + labelInfo.offsetX + 10;
  CListLabel *label = new CListLabel(x, labelInfo.offsetY, width - x - 18, height, labelInfo, LISTITEM_LABEL, "");
  m_controls.push_back(label);
  x = labelInfo2.offsetX ? labelInfo2.offsetX : m_width - 16;
  label = new CListLabel(x, labelInfo2.offsetY, x - iconWidth - 20, height, labelInfo2, LISTITEM_LABEL2, "");
  m_controls.push_back(label);
}

void CGUIListItemLayout::CreateThumbnailPanelLayouts(float width, float height, bool focused, const CImage &image, float texWidth, float texHeight, float thumbPosX, float thumbPosY, float thumbWidth, float thumbHeight, DWORD thumbAlign, CGUIImage::GUIIMAGE_ASPECT_RATIO thumbAspect, const CLabelInfo &labelInfo, bool hideLabels)
{
  m_width = width;
  m_height = height;
  m_focused = focused;
  float centeredPosX = (m_width - texWidth)*0.5f;
  // background texture
  CListTexture *tex = new CListTexture(centeredPosX, 0, texWidth, texHeight, image, CGUIImage::ASPECT_RATIO_STRETCH);
  m_controls.push_back(tex);
  // thumbnail
  float xOff = 0;
  float yOff = 0;
  if (thumbAlign != 0)
  {
    xOff += (texWidth - thumbWidth) * 0.5f;
    yOff += (texHeight - thumbHeight) * 0.5f;
    //if thumbPosX or thumbPosX != 0 the thumb will be bumped off-center
  }
  CListImage *thumb = new CListImage(thumbPosX + centeredPosX + xOff, thumbPosY + yOff, thumbWidth, thumbHeight, CImage(""), thumbAspect, LISTITEM_ICON);
  m_controls.push_back(thumb);
  // overlay
  CListImage *overlay = new CListImage(thumbPosX + centeredPosX + xOff + thumbWidth - 32, thumbPosY + yOff + thumbHeight - 32, 32, 32, CImage(""), thumbAspect, LISTITEM_OVERLAY);
  m_controls.push_back(overlay);
  // label
  if (hideLabels) return;
  CListLabel *label = new CListLabel(width*0.5f, texHeight, width, height, labelInfo, LISTITEM_LABEL, "");
  m_controls.push_back(label);
}
//#endif
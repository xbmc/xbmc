#pragma once

#include "GUIImage.h"
#include "GUILabelControl.h"

class CGUIListItem;

class CGUIListItemLayout
{
  class CListBase
  {
  public:
    CListBase(float posX, float posY, float width, float height);
    virtual ~CListBase();
    float m_posX;
    float m_posY;
    float m_width;
    float m_height;
    enum LIST_TYPE { LIST_LABEL, LIST_IMAGE, LIST_TEXTURE };
    LIST_TYPE m_type;
  };

  class CListLabel : public CListBase
  {
  public:
    CListLabel(float posX, float posY, float width, float height, const CLabelInfo &label, int info, const CStdString &contents);
    virtual ~CListLabel();
    CLabelInfo m_label;
    CStdStringW m_text;
    float m_renderX;  // render location
    float m_renderY;
    float m_renderW;
    float m_renderH;
    float m_textW;    // text width
    CScrollInfo m_scrollInfo;

    int m_info;
    vector<CInfoPortion> m_multiInfo;
  };

  class CListTexture : public CListBase
  {
  public:
    CListTexture(float posX, float posY, float width, float height, const CImage &image);
    virtual ~CListTexture();
    CGUIImage m_image;
  };

  class CListImage: public CListTexture
  {
  public:
    CListImage(float posX, float posY, float width, float height, int info);
    virtual ~CListImage();
    int m_info;
  };

public:
  CGUIListItemLayout();
  CGUIListItemLayout(const CGUIListItemLayout &from);
  ~CGUIListItemLayout();
  void LoadLayout(TiXmlElement *layout, bool focused);
  void Render(CGUIListItem *item);
  float Size(ORIENTATION orientation);
  bool Focused() const { return m_focused; };
  void ResetScrolling();

  void SetInvalid() { m_invalidated = true; };

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  void CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CImage &texture, float texHeight, float iconWidth, float iconHeight);
//#endif
protected:
  CListBase *CreateItem(TiXmlElement *child);
  void UpdateItem(CListBase *control, CGUIListItem *item);
  void RenderLabel(CListLabel *label, bool selected, bool scroll);

  vector<CListBase*> m_controls;
  typedef vector<CListBase*>::iterator iControls;
  typedef vector<CListBase*>::const_iterator ciControls;
  float m_width;
  float m_height;
  bool m_focused;
  bool m_invalidated;

  bool m_isPlaying;
};


#pragma once

#include "GUIBorderedImage.h"
#include "GUIListLabel.h"

class CGUIListItem;
class CFileItem;

class CGUIListItemLayout
{
  class CListBase
  {
  public:
    CListBase(int visibleCondition);
    virtual ~CListBase();

    enum LIST_TYPE { LIST_LABEL, LIST_IMAGE, LIST_TEXTURE };
    LIST_TYPE m_type;

    int m_visibleCondition;
    bool m_visible;
  };

  class CListLabel : public CListBase
  {
  public:
    CListLabel(float posX, float posY, float width, float height, int visibleCondition, const CLabelInfo &label, bool alwyasScroll, int info, const CStdString &contents, const vector<CAnimation> &animations);
    virtual ~CListLabel();

    CGUIListLabel m_label;
    int m_info;
    vector<CInfoPortion> m_multiInfo;
  };

  class CListTexture : public CListBase
  {
  public:
    CListTexture(float posX, float posY, float width, float height, int visibleCondition, const CImage &image, const CImage &borderImage, const FRECT &borderSize, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, DWORD aspectAlign, D3DCOLOR colorDiffuse, const vector<CAnimation> &animations);
    virtual ~CListTexture();
    CGUIBorderedImage m_image;
  };

  class CListImage: public CListTexture
  {
  public:
    CListImage(float posX, float posY, float width, float height, int visibleCondition, const CImage &image, const CImage &borderImage, const FRECT &borderSize, CGUIImage::GUIIMAGE_ASPECT_RATIO aspectRatio, DWORD aspectAlign, D3DCOLOR colorDiffuse, const vector<CAnimation> &animations, int info);
    virtual ~CListImage();
    int m_info;
  };

public:
  CGUIListItemLayout();
  CGUIListItemLayout(const CGUIListItemLayout &from);
  ~CGUIListItemLayout();
  void LoadLayout(TiXmlElement *layout, bool focused);
  void Render(CGUIListItem *item, DWORD parentID, DWORD time = 0);
  float Size(ORIENTATION orientation) const;
  bool Focused() const { return m_focused; };
  void ResetScrolling();
  void QueueAnimation(ANIMATION_TYPE animType);
  void ResetAnimation(ANIMATION_TYPE animType);

  void SetInvalid() { m_invalidated = true; };

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  void CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CImage &texture, const CImage &textureFocus, float texHeight, float iconWidth, float iconHeight, int nofocusCondition, int focusCondition);
  void CreateThumbnailPanelLayouts(float width, float height, bool focused, const CImage &image, float texWidth, float texHeight, float thumbPosX, float thumbPosY, float thumbWidth, float thumbHeight, DWORD thumbAlign, CGUIImage::GUIIMAGE_ASPECT_RATIO thumbAspect, const CLabelInfo &labelInfo, bool hideLabel);
//#endif

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  CListBase *CreateItem(TiXmlElement *child);
  void UpdateItem(CListBase *control, CFileItem *item, DWORD parentID);
  void RenderLabel(CListLabel *label, bool selected, bool scroll);
  void Update(CFileItem *item, DWORD parentID);

  vector<CListBase*> m_controls;
  typedef vector<CListBase*>::iterator iControls;
  typedef vector<CListBase*>::const_iterator ciControls;
  float m_width;
  float m_height;
  bool m_focused;
  bool m_invalidated;

  bool m_isPlaying;
};


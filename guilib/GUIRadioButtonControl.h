/*!
\file GUIRadioButtonControl.h
\brief 
*/

#pragma once
#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIRadioButtonControl :
      public CGUIButtonControl
{
public:
  CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId,
                         float posX, float posY, float width, float height,
                         const CImage& textureFocus, const CImage& textureNoFocus,
                         const CLabelInfo& labelInfo,
                         const CImage& radioFocus, const CImage& radioNoFocus);

  virtual ~CGUIRadioButtonControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetColorDiffuse(const CGUIInfoColor &color);
  virtual CStdString GetDescription() const;
  void SetRadioDimensions(float posX, float posY, float width, float height);
  void SetToggleSelect(int toggleSelect) { m_toggleSelect = toggleSelect; };
  bool IsSelected() const { return m_bSelected; };
protected:
  virtual void Update();
  CGUIImage m_imgRadioFocus;
  CGUIImage m_imgRadioNoFocus;
  float m_radioPosX;
  float m_radioPosY;
  int m_toggleSelect;
};

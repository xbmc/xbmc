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
                         int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                         const CStdString& strTextureFocus, const CStdString& strTextureNoFocus,
                         DWORD dwTextOffsetX, DWORD dwTextOffsetY, DWORD dwTextAlign,
                         const CStdString& strRadioFocus, const CStdString& strRadioNoFocus);

  virtual ~CGUIRadioButtonControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  const CStdString& GetTextureRadioFocusName() const { return m_imgRadioFocus.GetFileName(); };
  const CStdString& GetTextureRadioNoFocusName() const { return m_imgRadioNoFocus.GetFileName(); };
  virtual CStdString GetDescription() const;
protected:
  virtual void Update();
  CGUIImage m_imgRadioFocus;
  CGUIImage m_imgRadioNoFocus;
};

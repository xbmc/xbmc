/*!
\file GUICheckMarkControl.h
\brief 
*/

#ifndef CGUILIB_GUICHECKMARK_CONTROL_H
#define CGUILIB_GUICHECKMARK_CONTROL_H

#pragma once

#include "guiImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUICheckMarkControl: public CGUIControl
{
public:
  CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureCheckMark, const CImage& textureCheckMarkNF, float checkWidth, float checkHeight, const CLabelInfo &labelInfo);
  virtual ~CGUICheckMarkControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetColorDiffuse(D3DCOLOR color);

  void SetLabel(const string& strLabel);
  const string GetLabel() const { return m_strLabel; };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  void SetSelected(bool bOnOff);
  bool GetSelected() const;
  bool OnMouseClick(DWORD dwButton, const CPoint &point);

  void PythonSetLabel(const CStdString &strFont, const string &strText, DWORD dwTextColor);
  void PythonSetDisabledColor(DWORD dwDisabledColor);

protected:
  CGUIImage m_imgCheckMark;
  CGUIImage m_imgCheckMarkNoFocus;

  CLabelInfo m_label;
  CGUITextLayout m_textLayout;
  string m_strLabel;
  bool m_bSelected;
};
#endif

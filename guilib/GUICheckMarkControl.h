/*!
\file GUICheckMarkControl.h
\brief 
*/

#ifndef CGUILIB_GUICHECKMARK_CONTROL_H
#define CGUILIB_GUICHECKMARK_CONTROL_H

#pragma once

#include "GUIImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUICheckMarkControl: public CGUIControl
{
public:
  CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureCheckMark, const CStdString& strTextureCheckMarkNF, DWORD dwCheckWidth, DWORD dwCheckHeight, const CLabelInfo &labelInfo);
  virtual ~CGUICheckMarkControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);

  void SetLabel(const string& strLabel);
  const string GetLabel() const { return m_strLabel; };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  DWORD GetCheckMarkWidth() const { return m_imgCheckMark.GetWidth(); };
  DWORD GetCheckMarkHeight() const { return m_imgCheckMark.GetHeight(); };
  const CStdString& GetCheckMarkTextureName() const { return m_imgCheckMark.GetFileName(); };
  const CStdString& GetCheckMarkTextureNameNF() const { return m_imgCheckMarkNoFocus.GetFileName(); };
  void SetSelected(bool bOnOff);
  bool GetSelected() const;
  bool OnMouseClick(DWORD dwButton);

  void PythonSetLabel(const CStdString &strFont, const string &strText, DWORD dwTextColor);
  void PythonSetDisabledColor(DWORD dwDisabledColor);

protected:
  CGUIImage m_imgCheckMark;
  CGUIImage m_imgCheckMarkNoFocus;

  CLabelInfo m_label;
  string m_strLabel;
};
#endif

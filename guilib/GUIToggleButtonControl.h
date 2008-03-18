/*!
\file GUIToggleButtonControl.h
\brief 
*/

#ifndef GUILIB_GUITOGGLEBUTTONCONTROL_H
#define GUILIB_GUITOGGLEBUTTONCONTROL_H

#pragma once

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIToggleButtonControl : public CGUIButtonControl
{
public:
  CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureFocus, const CImage& textureNoFocus, const CImage& altTextureFocus, const CImage& altTextureNoFocus, const CLabelInfo &labelInfo);
  virtual ~CGUIToggleButtonControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetColorDiffuse(D3DCOLOR color);
  void SetLabel(const std::string& strLabel);
  void SetAltLabel(const std::string& label);
  virtual CStdString GetLabel() const;
  void SetToggleSelect(int toggleSelect) { m_toggleSelect = toggleSelect; };
  void SetAltClickActions(const std::vector<CStdString> &clickActions);

protected:
  virtual void OnClick();
  virtual void Update();
  CGUIButtonControl m_selectButton;
  int m_toggleSelect;
};
#endif

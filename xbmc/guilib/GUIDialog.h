/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIDialog.h
\brief
*/

#include "GUIWindow.h"
#include "WindowIDs.h"

#ifdef TARGET_WINDOWS_STORE
#pragma pack(push, 8)
#endif
enum class DialogModalityType
{
  MODELESS,
  MODAL
};
#ifdef TARGET_WINDOWS_STORE
#pragma pack(pop)
#endif

/*!
 \ingroup winmsg
 \brief
 */
class CGUIDialog :
      public CGUIWindow
{
public:
  CGUIDialog(int id, const std::string &xmlFile, DialogModalityType modalityType = DialogModalityType::MODAL);
  ~CGUIDialog(void) override;

  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;

  void Open(const std::string &param = "");
  void Open(bool bProcessRenderLoop, const std::string& param = "");

  bool OnBack(int actionID) override;

  bool IsDialogRunning() const override { return m_active; }
  bool IsDialog() const override { return true; }
  bool IsModalDialog() const override { return m_modalityType == DialogModalityType::MODAL; }
  virtual DialogModalityType GetModalityType() const { return m_modalityType; }

  void SetAutoClose(unsigned int timeoutMs);
  void ResetAutoClose(void);
  void CancelAutoClose(void);
  bool IsAutoClosed(void) const { return m_bAutoClosed; }
  void SetSound(bool OnOff) { m_enableSound = OnOff; }
  bool IsSoundEnabled() const override { return m_enableSound; }

protected:
  bool Load(TiXmlElement *pRootElement) override;
  void SetDefaults() override;
  void OnWindowLoaded() override;
  using CGUIWindow::UpdateVisibility;
  virtual void UpdateVisibility();

  virtual void Open_Internal(bool bProcessRenderLoop, const std::string &param = "");
  void OnDeinitWindow(int nextWindowID) override;

  void ProcessRenderLoop(bool renderOnly = false);

  bool m_wasRunning; ///< \brief true if we were running during the last DoProcess()
  bool m_autoClosing;
  bool m_enableSound;
  unsigned int m_showStartTime;
  unsigned int m_showDuration;
  bool m_bAutoClosed;
  DialogModalityType m_modalityType;
};

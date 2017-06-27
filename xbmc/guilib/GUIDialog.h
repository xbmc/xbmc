/*!
\file GUIDialog.h
\brief
*/

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindow.h"
#include "WindowIDs.h"

enum class DialogModalityType
{
  MODELESS,
  MODAL,
  PARENTLESS_MODAL
};

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
  
  bool OnBack(int actionID) override;

  bool IsDialogRunning() const override { return m_active; };
  bool IsDialog() const override { return true;};
  bool IsModalDialog() const override { return m_modalityType == DialogModalityType::MODAL || m_modalityType == DialogModalityType::PARENTLESS_MODAL; };
  virtual DialogModalityType GetModalityType() const { return m_modalityType; };

  void SetAutoClose(unsigned int timeoutMs);
  void ResetAutoClose(void);
  void CancelAutoClose(void);
  bool IsAutoClosed(void) const { return m_bAutoClosed; };
  void SetSound(bool OnOff) { m_enableSound = OnOff; };
  bool IsSoundEnabled() const override { return m_enableSound; };

protected:
  bool Load(TiXmlElement *pRootElement) override;
  void SetDefaults() override;
  void OnWindowLoaded() override;
  using CGUIWindow::UpdateVisibility;
  virtual void UpdateVisibility();

  virtual void Open_Internal(const std::string &param = "");
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

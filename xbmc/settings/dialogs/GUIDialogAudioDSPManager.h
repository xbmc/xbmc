#pragma once
/*
 *      Copyright (C) 2012-2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_adsp_types.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPMode.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

class CGUIDialogBusy;

namespace ActiveAE
{
  class CGUIDialogAudioDSPManager : public CGUIDialog
  {
  public:
    CGUIDialogAudioDSPManager(void);
    virtual ~CGUIDialogAudioDSPManager(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool OnAction(const CAction& action);
    virtual void OnWindowLoaded(void);
    virtual void OnWindowUnload(void);
    virtual bool HasListItems() const { return true; };

  protected:
    virtual void OnInitWindow();
    virtual void OnDeinitWindow(int nextWindowID);

    virtual bool OnPopupMenu(int iItem, int listType);
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button, int listType);

    virtual bool OnActionMove(const CAction &action);

    virtual bool OnMessageClick(CGUIMessage &message);

    bool OnClickListAvailable(CGUIMessage &message);
    bool OnClickListActive(CGUIMessage &message);
    bool OnClickRadioContinousSaving(CGUIMessage &message);
    bool OnClickApplyChanges(CGUIMessage &message);
    bool OnClickClearActiveModes(CGUIMessage &message);

    void SetItemsUnchanged(void);

  private:
    void Clear(void);
    void Update(void);
    void SaveList(void);
    void Renumber(void);
    bool UpdateDatabase(CGUIDialogBusy* pDlgBusy);
    void SetSelectedModeType(void);

    //! helper function prototypes
    static void                 helper_LogError(const char *function);
    static int                  helper_TranslateModeType(std::string ModeString);
    static CFileItem           *helper_CreateModeListItem(CActiveAEDSPModePtr &ModePointer, AE_DSP_MENUHOOK_CAT &MenuHook, int *ContinuesNo);
    static int                  helper_GetDialogId(CActiveAEDSPModePtr &ModePointer, AE_DSP_MENUHOOK_CAT &MenuHook, AE_DSP_ADDON &Addon, std::string AddonName);
    static AE_DSP_MENUHOOK_CAT  helper_GetMenuHookCategory(int CurrentType);

    bool m_bMovingMode;
    bool m_bContainsChanges;
    bool m_bContinousSaving;    // if true, all settings are directly saved

    int m_iCurrentType;
    int m_iSelected[AE_DSP_MODE_TYPE_MAX];

    CFileItemList* m_activeItems[AE_DSP_MODE_TYPE_MAX];
    CFileItemList* m_availableItems[AE_DSP_MODE_TYPE_MAX];

    CGUIViewControl m_availableViewControl;
    CGUIViewControl m_activeViewControl;
  };
}

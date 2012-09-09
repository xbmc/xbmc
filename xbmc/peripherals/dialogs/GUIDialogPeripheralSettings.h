#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "guilib/GUIDialog.h"
#include "settings/GUIDialogSettings.h"
#include "FileItem.h"

namespace PERIPHERALS
{
  class CGUIDialogPeripheralSettings : public CGUIDialogSettings
  {
  public:
    CGUIDialogPeripheralSettings(void);
    virtual ~CGUIDialogPeripheralSettings(void);

    virtual void SetFileItem(CFileItemPtr item);
    virtual bool OnMessage(CGUIMessage &message);
  protected:
    virtual void CreateSettings();
    virtual void OnOkay(void);
    virtual void ResetDefaultSettings(void);
    virtual void UpdatePeripheralSettings(void);

    CFileItem *                      m_item;
    bool                             m_bIsInitialising;
    std::map<CStdString, bool>       m_boolSettings;
    std::map<CStdString, float>      m_intSettings;
    std::map<CStdString, int>        m_intTextSettings;
    std::map<CStdString, float>      m_floatSettings;
    std::map<CStdString, CStdString> m_stringSettings;
  };
}

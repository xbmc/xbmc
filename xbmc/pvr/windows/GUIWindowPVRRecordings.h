#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "video/VideoThumbLoader.h"
#include "video/VideoDatabase.h"

#include "pvr/PVRSettings.h"

#include "GUIWindowPVRBase.h"

namespace PVR
{
  class CGUIWindowPVRRecordingsBase : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRRecordingsBase(bool bRadio, int id, const std::string &xmlFile);
    virtual ~CGUIWindowPVRRecordingsBase();

    void OnWindowLoaded() override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
    void UpdateButtons() override;

  protected:
    std::string GetDirectoryPath(void) override;
    void OnPrepareFileItems(CFileItemList &items) override;
    bool GetFilteredItems(const std::string &filter, CFileItemList &items) override;

  private:
    bool OnContextButtonDeleteAll(CFileItem *item, CONTEXT_BUTTON button);

    CVideoThumbLoader m_thumbLoader;
    CVideoDatabase m_database;
    bool m_bShowDeletedRecordings;
    CPVRSettings m_settings;
  };

  class CGUIWindowPVRTVRecordings : public CGUIWindowPVRRecordingsBase
  {
  public:
    CGUIWindowPVRTVRecordings() : CGUIWindowPVRRecordingsBase(false, WINDOW_TV_RECORDINGS, "MyPVRRecordings.xml") {}
  };

  class CGUIWindowPVRRadioRecordings : public CGUIWindowPVRRecordingsBase
  {
  public:
    CGUIWindowPVRRadioRecordings() : CGUIWindowPVRRecordingsBase(true, WINDOW_RADIO_RECORDINGS, "MyPVRRecordings.xml") {}
  };
}

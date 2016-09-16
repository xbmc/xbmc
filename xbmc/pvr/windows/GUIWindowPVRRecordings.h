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

#include "GUIWindowPVRBase.h"

namespace PVR
{
  class CGUIWindowPVRRecordings : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRRecordings(bool bRadio);
    virtual ~CGUIWindowPVRRecordings(void);

    static std::string GetResumeString(const CFileItem& item);

    virtual void OnWindowLoaded() override;
    virtual bool OnMessage(CGUIMessage& message) override;
    virtual bool OnAction(const CAction &action) override;
    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
    virtual void UpdateButtons(void) override;

  protected:
    virtual std::string GetDirectoryPath(void) override;
    virtual void OnPrepareFileItems(CFileItemList &items) override;

  private:
    bool ActionDeleteRecording(CFileItem *item);
    bool OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonUndelete(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonDeleteAll(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonMarkWatched(const CFileItemPtr &item, CONTEXT_BUTTON button);

    CVideoThumbLoader m_thumbLoader;
    CVideoDatabase m_database;
    bool m_bShowDeletedRecordings;
  };
}

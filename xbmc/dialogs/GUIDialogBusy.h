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


class CGUIDialogBusy: public CGUIDialog
{
public:
  CGUIDialogBusy(void);
  virtual ~CGUIDialogBusy(void);
  virtual bool OnBack(int actionID);
  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  /*! \brief set the current progress of the busy operation
   \param progress a percentage of progress
   */
  void SetProgress(float progress);

  bool IsCanceled() { return m_bCanceled; }
protected:
  virtual void Show_Internal(); // modeless'ish
  bool m_bCanceled;
  bool m_bLastVisible;
  float m_progress; ///< current progress
};

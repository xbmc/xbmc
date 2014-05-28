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

#ifndef DIALOGS_FILEITEM_H_INCLUDED
#define DIALOGS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef DIALOGS_GUIDIALOGYESNO_H_INCLUDED
#define DIALOGS_GUIDIALOGYESNO_H_INCLUDED
#include "GUIDialogYesNo.h"
#endif

#ifndef DIALOGS_UTILS_VARIANT_H_INCLUDED
#define DIALOGS_UTILS_VARIANT_H_INCLUDED
#include "utils/Variant.h"
#endif


class CGUIDialogPlayEject : public CGUIDialogYesNo
{
public:
  CGUIDialogPlayEject();
  virtual ~CGUIDialogPlayEject();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void FrameMove();

  static bool ShowAndGetInput(const CFileItem & item, unsigned int uiAutoCloseTime = 0);

protected:
  virtual void OnInitWindow();
};

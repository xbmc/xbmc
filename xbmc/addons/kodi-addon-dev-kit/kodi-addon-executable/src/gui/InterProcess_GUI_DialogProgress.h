#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_DialogProgress
  {
    GUIHANDLE Dialogs_Progress_New();
    void Dialogs_Progress_Delete(GUIHANDLE handle);
    void Dialogs_Progress_Open(GUIHANDLE handle);
    void Dialogs_Progress_SetHeading(GUIHANDLE handle, const std::string& title);
    void Dialogs_Progress_SetLine(GUIHANDLE handle, unsigned int iLine, const std::string& line);
    void Dialogs_Progress_SetCanCancel(GUIHANDLE handle, bool bCanCancel);
    bool Dialogs_Progress_IsCanceled(GUIHANDLE handle) const;
    void Dialogs_Progress_SetPercentage(GUIHANDLE handle, int iPercentage);
    int Dialogs_Progress_GetPercentage(GUIHANDLE handle) const;
    void Dialogs_Progress_ShowProgressBar(GUIHANDLE handle, bool bOnOff);
    void Dialogs_Progress_SetProgressMax(GUIHANDLE handle, int iMax);
    void Dialogs_Progress_SetProgressAdvance(GUIHANDLE handle, int nSteps);
    bool Dialogs_Progress_Abort(GUIHANDLE handle);
  };

}; /* extern "C" */

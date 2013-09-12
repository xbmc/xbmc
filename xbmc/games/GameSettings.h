/*
 *      Copyright (C) 2012-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "settings/ISettingCallback.h"

class CSetting;

namespace GAMES
{
  class CGameSettings : public ISettingCallback
  {
  public:
    static CGameSettings &Get();
    
    // Inherited from ISettingCallback
    virtual void OnSettingAction(const CSetting *setting);

  protected:
    virtual ~CGameSettings() { }

  private: // non-copyable
    CGameSettings() { }
    CGameSettings(const CGameSettings&);
    CGameSettings& operator=(const CGameSettings&);
  };
} // namespace GAMES

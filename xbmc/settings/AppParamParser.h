/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _WIN32
#include "WIN32Util.h"
#endif

class CAppParamParser
{
  public:
    CAppParamParser();
#ifdef _WIN32
    void Parse(LPWSTR *szArglist, int nArgs);
#else
    void Parse(char* argv[]);
#endif
  private:
    void DisplayHelp();
    void EnableDebugMode();
    void SetStartFullScreen();
    void SetIsStandalone();
    void SetPortable();
};
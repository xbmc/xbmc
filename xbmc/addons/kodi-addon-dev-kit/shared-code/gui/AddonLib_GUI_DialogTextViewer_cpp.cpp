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

#include "InterProcess.h"
#include "kodi/api2/gui/DialogTextViewer.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
namespace DialogTextViewer
{

  void Show(
          const std::string& heading,
          const std::string& text)
  {
    return g_interProcess.Dialogs_TextViewer_Show(heading, text);
  }

}; /* namespace DialogTextViewer */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */

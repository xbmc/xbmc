/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "PlayerOperations.h"
#include "Application.h"
#include "Util.h"
#include "PlayListPlayer.h"
#include "guilib/GUIWindowManager.h"

using namespace JSONRPC;

JSON_STATUS CPlayerOperations::GetActivePlayers(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  result["video"] = g_application.IsPlayingVideo();
  result["audio"] = g_application.IsPlayingAudio();
  result["picture"] = g_windowManager.IsWindowActive(WINDOW_SLIDESHOW);

  return OK;
}

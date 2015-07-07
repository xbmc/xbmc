/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "dacp.h"
#include "filesystem/File.h"

#define AIRTUNES_DACP_CMD_URI "ctrl-int/1/"

// AirTunes related DACP implementation taken from http://nto.github.io/AirPlay.html#audio-remotecontrol

CDACP::CDACP(const std::string &active_remote_header, const std::string &hostname, int port)
{
  m_dacpUrl.SetHostName(hostname);
  m_dacpUrl.SetPort(port);
  m_dacpUrl.SetProtocol("http");
  m_dacpUrl.SetProtocolOption("Active-Remote", active_remote_header);
}

void CDACP::SendCmd(const std::string &cmd)
{
  m_dacpUrl.SetFileName(AIRTUNES_DACP_CMD_URI + cmd);
  // issue the command
  XFILE::CFile file;
  file.Open(m_dacpUrl);
  file.Write(NULL, 0);
}

void CDACP::BeginFwd()
{
  SendCmd("beginff");
}

void CDACP::BeginRewnd()
{
  SendCmd("beginrew");
}

void CDACP::ToggleMute()
{
  SendCmd("mutetoggle");
}

void CDACP::NextItem()
{
  SendCmd("nextitem");
}

void CDACP::PrevItem()
{
  SendCmd("previtem");
}

void CDACP::Pause()
{
  SendCmd("pause");
}

void CDACP::PlayPause()
{
  SendCmd("playpause");
}

void CDACP::Play()
{
  SendCmd("play");
}

void CDACP::Stop()
{
  SendCmd("stop");
}

void CDACP::PlayResume()
{
  SendCmd("playresume");
}

void CDACP::ShuffleSongs()
{
  SendCmd("shuffle_songs");
}

void CDACP::VolumeDown()
{
  SendCmd("volumedown");
}

void CDACP::VolumeUp()
{
  SendCmd("volumeup");
}

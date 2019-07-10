/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/IMsgTargetCallback.h"
#include "messaging/IMessageTarget.h"
#include "playlists/PlayListTypes.h"

#include <chrono>
#include <map>
#include <memory>

class CAction;
class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;

class CVariant;

using namespace PLAYLIST;

class IMediaPlayer 
{
public:
  IMediaPlayer() = default;
  virtual ~IMediaPlayer() = default;
  virtual void SetShuffle(Id playlistId, bool bYesNo, bool bNotify /* = false */, int m_iCurrentSong) = 0;
};

class CMediaPlayerMusic : public IMediaPlayer 
{
public:
  CMediaPlayerMusic()=default;

  void SetShuffle(Id playlistId, bool bYesNo, bool bNotify /* = false */, int m_iCurrentSong) override;
};

class CMediaPlayerVideo : public IMediaPlayer //B
{
public:
  CMediaPlayerVideo() = default;

  void SetShuffle(Id playlistId, bool bYesNo, bool bNotify /* = false */, int m_iCurrentSong) override;
};
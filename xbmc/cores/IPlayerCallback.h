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

#pragma once

class CFileItem;

class IPlayerCallback
{
public:
  virtual ~IPlayerCallback() = default;
  virtual void OnPlayBackEnded() = 0;
  virtual void OnPlayBackStarted(const CFileItem &file) = 0;
  virtual void OnPlayBackPaused() {};
  virtual void OnPlayBackResumed() {};
  virtual void OnPlayBackStopped() = 0;
  virtual void OnPlayBackError() = 0;
  virtual void OnQueueNextItem() = 0;
  virtual void OnPlayBackSeek(int64_t iTime, int64_t seekOffset) {};
  virtual void OnPlayBackSeekChapter(int iChapter) {};
  virtual void OnPlayBackSpeedChanged(int iSpeed) {};
};

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

// interface for registering into windowing
// to get notified about display events
// interface only, does not control lifetime of the object
class IDispResource
{
public:
  virtual void OnLostDisplay() {};
  virtual void OnResetDisplay() {};
  virtual void OnAppFocusChange(bool focus) {};
};

// interface used by clients to register into render loop
// interface only, does not control lifetime of the object
class IRenderLoop
{
public:
  virtual void FrameMove() = 0;
};

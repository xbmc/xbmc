/*
 *      Copyright (C) 2017 Team Kodi
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

#include "games/GameTypes.h"
#include "threads/Thread.h"

namespace KODI
{
namespace RETRO
{
  class CRetroPlayerAutoSave : protected CThread
  {
  public:
    explicit CRetroPlayerAutoSave(GAME::CGameClient &gameClient);

    ~CRetroPlayerAutoSave() override;

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    // Construction parameter
    GAME::CGameClient &m_gameClient;
  };
}
}

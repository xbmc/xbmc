#pragma once
/*
 *      Copyright (C) 2010 Marcel Groothuis
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "thread.h"

class CKeepAliveThread : cThread
{
public:
  CKeepAliveThread();
  virtual ~CKeepAliveThread(void);

  bool IsThreadRunning() { return Active(); }
  long StopThread(unsigned long dwTimeoutMilliseconds  = 1000) { Cancel(dwTimeoutMilliseconds / 1000); return 0; }
  long StartThread(void) { Start(); return 0; }


private:
  virtual void Action();
};


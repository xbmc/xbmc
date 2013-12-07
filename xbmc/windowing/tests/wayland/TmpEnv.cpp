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
#include <stdlib.h>

#include "TmpEnv.h"

TmpEnv::TmpEnv(const char *env,
               const char *val) :
  m_env(env),
  m_previous(getenv(env))
{
  setenv(env, val, 1);
}

TmpEnv::~TmpEnv()
{
  if (m_previous)
    setenv(m_env, m_previous, 1);
  else
    unsetenv(m_env);
}

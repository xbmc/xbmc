/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

 /* NOTE:
  *
  * The following code is taken and copied from VDR streamdev plugin.
  */

#include "select.h"
#include "tools.h"
#include <sys/types.h>
#include <time.h>
#include <errno.h>

cTBSelect::cTBSelect(void)
{
	Clear();
}

cTBSelect::~cTBSelect()
{
}

int cTBSelect::Select(unsigned int TimeoutMs)
{
	struct timeval tv;
	ssize_t res = 0;
	int ms;

	tv.tv_usec = (TimeoutMs % 1000) * 1000;
	tv.tv_sec = TimeoutMs / 1000;
	memcpy(m_FdsResult, m_FdsQuery, sizeof(m_FdsResult));

	if (TimeoutMs == 0)
		return __select(m_MaxFiled + 1, &m_FdsResult[0], &m_FdsResult[1], NULL, &tv);

	cTimeMs starttime;
	ms = TimeoutMs;
	while (ms > 0 && (res = __select(m_MaxFiled + 1, &m_FdsResult[0], &m_FdsResult[1], NULL, &tv)) == -1 && errno == EINTR)
	{
		ms = TimeoutMs - starttime.Elapsed();
		tv.tv_usec = (ms % 1000) * 1000;
		tv.tv_sec = ms / 1000;
		memcpy(m_FdsResult, m_FdsQuery, sizeof(m_FdsResult));
	}
	if (ms <= 0 || res == 0)
	{
		errno = ETIMEDOUT;
		return -1;
	}
	return res;
}

int cTBSelect::Select(void)
{
	ssize_t res;
	do
	{
		memcpy(m_FdsResult, m_FdsQuery, sizeof(m_FdsResult));
	} while ((res = __select(m_MaxFiled + 1, &m_FdsResult[0], &m_FdsResult[1], NULL, NULL)) == -1 && errno == EINTR);
	return res;
}

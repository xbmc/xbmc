#pragma once
/*
*      Copyright (C) 2005-2008 Team XBMC
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

#include "DateTime.h"
#include "TVChannel.h"

class IEPGObserver
{
public:
  virtual ~IEPGObserver() {}
  virtual void OnChannelUpdated(unsigned channel) = 0;
};

class CEPG
{
public:
  CEPG(DWORD sourceID, CDateTime start, CDateTime end);
  CEPG(const CEPG& rhs);

  void AddChannel(DWORD sourceID, CTVChannel channel);

  CDateTime m_start;
  CDateTime m_end;

  const VECCHANNELS* GetGrid() { return m_grid; };
  const CDateTime GetStart() { return m_start; };
  const CDateTime GetEnd() { return m_end; };

  bool Save();
  bool Sort();

private:
  VECCHANNELS* m_grid;
};

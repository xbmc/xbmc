#ifndef __XLCDPROC_H__
#define __XLCDPROC_H__

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

#include <SDL/SDL_thread.h>
#include "../utils/LCD.h"

#define MAX_ROWS 20

class XLCDproc : public ILCD
{
public:
  XLCDproc();
  virtual ~XLCDproc(void);
  virtual void Initialize();
  virtual void Stop();
  virtual void SetBackLight(int iLight);
  virtual void SetContrast(int iContrast);
protected:
  virtual void Process();
  virtual void SetLine(int iLine, const CStdString& strLine);
  unsigned int m_iColumns;        // display columns for each line
  unsigned int m_iRows;       // total number of rows
  unsigned int m_iRow1adr ;
  unsigned int m_iRow2adr ;
  unsigned int m_iRow3adr ;
  unsigned int m_iRow4adr ;
  unsigned int m_iActualpos;        // actual cursor possition
  int          m_iBackLight;
  int          m_iLCDContrast;
  bool         m_bUpdate[MAX_ROWS];
  CStdString   m_strLine[MAX_ROWS];
  int          m_iPos[MAX_ROWS];
  DWORD        m_dwSleep[MAX_ROWS];
  CEvent       m_event;
  int          sockfd;

};

#endif

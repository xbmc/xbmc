//-----------------------------------------------------------------------------
// File: DelayController.h
//
// Desc: CDelayController header file
//
// Hist:
//
//
//-----------------------------------------------------------------------------

#ifndef __DELAYCONTROLLER_H__
#define __DELAYCONTROLLER_H__

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

#define DC_SKIP       0x0001
#define DC_UP       0x0002
#define DC_DOWN       0x0004
#define DC_LEFT       0x0008
#define DC_RIGHT      0x0010
#define DC_LEFTTRIGGER     0x0020
#define DC_RIGHTTRIGGER     0x0040



class CDelayController
{
public:
  CDelayController( DWORD dwMoveDelay, DWORD dwRepeatDelay );
  WORD DpadInput( WORD wDpad, bool bLeftTrigger, bool bRightTrigger );

  WORD StickInput( int x, int y );
  WORD DirInput( WORD wDir );
  void SetDelays( DWORD dwMoveDelay, DWORD dwRepeatDelay );

protected:
  WORD m_wLastDir;
  DWORD m_dwTimer;
  int m_iCount;
  DWORD m_dwMoveDelay;
  DWORD m_dwRepeatDelay;
  DWORD m_dwLastTime;
};


#endif // __DELAYCONTROLLER_H__

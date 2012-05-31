/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#pragma once
#include "music/Song.h"

/*! Base virtual class for interaction with CueParser.
 * Implementation MUST provide CUE-data to parser and can collect useful information from parser.
 * CueParser call different virtual methods of callback for different reasons.
 * 
 * All \b on* methods can return true if parser need continue otherwise methods returns false.
 */
class CueParserCallback
{
public:
  /*! Implementation should fill \b dataLie with next portion of CUE-data.
   */
  virtual bool onDataNeeded(CStdString& dataLine) = 0;
  /*! This method called every time when parser found FILE record in cue-sheet.
   * Callback can modify filePath.
   */
  virtual bool onFile(CStdString& filePath) = 0;
  /*! This method called for every track found by parser.
   */
  virtual bool onTrackReady(CSong& song) = 0;
  virtual ~CueParserCallback() {}
};
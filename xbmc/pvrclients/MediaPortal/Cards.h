#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>
#include <string>
#include <ctime>

using namespace std;

/**
 * MediaPortal TVServer card settings ("card" table in the database)
 */
typedef struct Card
{
  int      IdCard;
  string   DevicePath;
  string   Name;
  int      Priority;
  bool     GrabEPG;
  time_t   LastEpgGrab;
  string   RecordingFolder;
  int      IdServer;
  bool     Enabled;
  int      CamType;
  string   TimeshiftingFolder;
  int      RecordingFormat;
  int      DecryptLimit;
  bool     Preload;
  bool     CAM;
  int      NetProvider;
  bool     StopGraph;
} Card;

class CCards: public vector<Card>
{
  public:

    /**
     * \brief Parse the multi-line string response from the TVServerXBMC plugin command "GetCardSettings"
     * The data is stored in "struct Card" item.
     * 
     * \param lines Vector with response lines
     * \return True on success, False on failure
     */
    bool ParseLines(vector<string>& lines);
};

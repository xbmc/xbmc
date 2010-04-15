/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <vector>
#include "channels.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

cChannel::cChannel(const PVR_CHANNEL *Channel)
{

}

cChannel::cChannel()
{
  name = "";
  uid = 0;
  external_id = 0;
}

cChannel::~cChannel()
{
}

bool cChannel::Parse(const std::string& data)
{
  vector<string> fields;

  Tokenize(data, fields, "|");

  if (fields.size() >= 4)
  {
    //Expected format:
    //ListTVChannels, ListRadioChannels
    //0 = channel uid
    //1 = channel external id/number
    //2 = channel name
    //3 = isencrypted ("0"/"1")

    uid = atoi(fields[0].c_str());
    external_id = atoi(fields[1].c_str());
    name = fields[2];
    encrypted = (strncmp(fields[3].c_str(), "1", 1) == 0);
    return true;
  } else {
    return false;
  }
}

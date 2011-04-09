/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *
 */

#include <vector>
#include "channel.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

cChannel::cChannel()
{
  name = "";
  guid = "";
  type = ForTheRecord::Television;
  lcn = 0;
  id = 0;
  guidechannelid = "";
}

cChannel::~cChannel()
{
}

bool cChannel::Parse(const Json::Value& data)
{
    //Json::printValueTree(data);

    name = data["DisplayName"].asString();
    type = (ForTheRecord::ChannelType) data["ChannelType"].asInt();
    lcn = data["LogicalChannelNumber"].asInt();
    // Useless for XBMC: a unique id as 128 bit GUID string. XBMC accepts only integers here...
    guid = data["ChannelId"].asString();
    guidechannelid = data["GuideChannelId"].asString();

    // Not needed...
    //["BroadcastStart"] //string
    //["BroadcastStop"] //string
    //["DefaultPostRecordSeconds"] //int
    //["DefaultPreRecordSeconds"] //int
    //["Sequence"] //int =0
    //["Version"] //int =0
    //["VisibleInGuide"] //boolean =true

    return true;
}

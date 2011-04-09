#pragma once
/*
 *      Copyright (C) 2010 Marcel Groothuis
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

#include "libXBMC_pvr.h"
#include <string>
#include <json/json.h>
#include "fortherecordrpc.h"

class cChannel
{
private:
  std::string name;
  std::string guid;
  std::string guidechannelid;
  ForTheRecord::ChannelType type;
  int lcn;
  int id;

public:
  cChannel();
  virtual ~cChannel();

  bool Parse(const Json::Value& data);
  const char *Name(void) const { return name.c_str(); }
  const std::string& Guid(void) const { return guid; }
  int LCN(void) const { return lcn; }
  ForTheRecord::ChannelType Type(void) const { return type; }
  int ID(void) const { return id; }
  void SetID(int new_id) { id = new_id; }
  const std::string& GuideChannelID(void) const { return guidechannelid; };
};

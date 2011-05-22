/*
 *      Copyright (C) 2010 Marcel Groothuis
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
#include <stdio.h>

using namespace std;

#include "epg.h"
#include "utils.h"
#include "client.h"
#include "pvrclient-fortherecord.h"

cEpg::cEpg()
{
  m_starttime       = 0;
  m_endtime         = 0;
}

cEpg::~cEpg()
{
}

void cEpg::Reset()
{
  m_guideprogramid.clear();
  m_title.clear();
  m_subtitle.clear();
  m_description.clear();

  m_starttime       = 0;
  m_endtime         = 0;
}

bool cEpg::Parse(const Json::Value& data)
{
  try
  {
    int offset;
    // All possible fields:
    //.Category=""
    //.EpisodeNumber=null
    //.EpisodeNumberDisplay=""
    //.EpisodeNumberTotal=null
    //.EpisodePart=null
    //.EpisodePartTotal=null
    //.GuideChannelId="26aa19b2-9d5d-4549-9ad8-ab6b908d6127"
    //.GuideProgramId="5bd17a57-f1f7-df11-862d-005056c00008"
    //.IsPremiere=false
    //.IsRepeat=false
    //.Rating=""
    //.SeriesNumber=null
    //.StarRating=null
    //.StartTime="/Date(1290896700000+0100)/" Database: 2010-11-27 23:25:00
    //.StopTime="/Date(1290899100000+0100)/"  Database: 2010-11-28 00:05:00
    //.SubTitle=""
    //.Title="NOS Studio Sport"
    //.VideoAspect=0
    m_guideprogramid = data["GuideProgramId"].asString();
    m_title = data["Title"].asString();
    m_subtitle = data["SubTitle"].asString();
    m_description = data["Description"].asString();

    // Dates are returned in a WCF compatible format ("/Date(9991231231+0100)/")
    std::string starttime = data["StartTime"].asString();
    std::string endtime = data["StopTime"].asString();

    m_starttime = ForTheRecord::WCFDateToTimeT(starttime, offset);
    m_endtime = ForTheRecord::WCFDateToTimeT(endtime, offset);

    //XBMC->Log(LOG_DEBUG, "Program: %s,%s Start: %s", m_title.c_str(), m_subtitle.c_str(), ctime(&m_starttime));
    //XBMC->Log(LOG_DEBUG, "End: %s", ctime(&m_endtime));

    return true;
  }
  catch(std::exception &e)
  {
    XBMC->Log(LOG_ERROR, "Exception '%s' during parse EPG json data.", e.what());
  }

  return false;
}


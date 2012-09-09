/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if _WIN32
#define PCRE_STATIC
#endif
#include <pcrecpp.h>
#include <cmath>
#include "FTPParse.h"

CFTPParse::CFTPParse()
{
  m_name = "";
  m_flagtrycwd = 0;
  m_flagtryretr = 0;
  m_size = 0;
  m_time = 0;
}

string CFTPParse::getName()
{
  return m_name;
}

int CFTPParse::getFlagtrycwd()
{
  return m_flagtrycwd;
}

int CFTPParse::getFlagtryretr()
{
  return m_flagtryretr;
}

uint64_t CFTPParse::getSize()
{
  return m_size;
}

time_t CFTPParse::getTime()
{
  return m_time;
}

void CFTPParse::setTime(string str)
{
  /* Variables used to capture patterns via the regexes */
  string month;
  string day;
  string year;
  string hour;
  string minute;
  string second;
  string am_or_pm;

  /* time struct used to set the time_t variable */
  struct tm time_struct = {};

  /* Regex to read Unix, NetWare and NetPresenz time format */
  pcrecpp::RE unix_re("^([A-Za-z]{3})" // month
    "\\s+(\\d{1,2})" // day of month
    "\\s+([:\\d]{4,5})$" // time of day or year
  );

  /* Regex to read MultiNet time format */
  pcrecpp::RE multinet_re("^(\\d{1,2})" // day of month
    "-([A-Za-z]{3})" // month
    "-(\\d{4})" // year
    "\\s+(\\d{2})" // hour
    ":(\\d{2})" // minute
    "(:(\\d{2}))?$" // second
  );

  /* Regex to read MSDOS time format */
  pcrecpp::RE msdos_re("^(\\d{2})" // month
    "-(\\d{2})" // day of month
    "-(\\d{2})" // year
    "\\s+(\\d{2})" // hour
    ":(\\d{2})" // minute
    "([AP]M)$" // AM or PM
  );

  if (unix_re.FullMatch(str, &month, &day, &year))
  {
    /* set the month */
    if (pcrecpp::RE("jan",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 0;
    else if (pcrecpp::RE("feb",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 1;
    else if (pcrecpp::RE("mar",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 2;
    else if (pcrecpp::RE("apr",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 3;
    else if (pcrecpp::RE("may",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 4;
    else if (pcrecpp::RE("jun",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 5;
    else if (pcrecpp::RE("jul",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 6;
    else if (pcrecpp::RE("aug",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 7;
    else if (pcrecpp::RE("sep",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 8;
    else if (pcrecpp::RE("oct",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 9;
    else if (pcrecpp::RE("nov",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 10;
    else if (pcrecpp::RE("dec",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 11;

    /* set the day of the month */
    time_struct.tm_mday = atoi(day.c_str());

    time_t t = time(NULL);
    struct tm *current_time = localtime(&t);
    if (pcrecpp::RE("(\\d{2}):(\\d{2})").FullMatch(year, &hour, &minute))
    {
      /* set the hour and minute */
      time_struct.tm_hour = atoi(hour.c_str());
      time_struct.tm_min = atoi(minute.c_str());

      /* set the year */
      if ((current_time->tm_mon - time_struct.tm_mon < 0) ||
         ((current_time->tm_mon - time_struct.tm_mon == 0) &&
          (current_time->tm_mday - time_struct.tm_mday < 0)))
        time_struct.tm_year = current_time->tm_year - 1;
      else
        time_struct.tm_year = current_time->tm_year;
    }
    else
    {
      /* set the year */
      time_struct.tm_year = atoi(year.c_str()) - 1900;
    }

    /* set the day of the week */
    time_struct.tm_wday = getDayOfWeek(time_struct.tm_mon + 1,
                                   time_struct.tm_mday,
                                   time_struct.tm_year + 1900);
  }
  else if (multinet_re.FullMatch(str, &day, &month, &year,
                            &hour, &minute, (void*)NULL, &second))
  {
    /* set the month */
    if (pcrecpp::RE("jan",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 0;
    else if (pcrecpp::RE("feb",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 1;
    else if (pcrecpp::RE("mar",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 2;
    else if (pcrecpp::RE("apr",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 3;
    else if (pcrecpp::RE("may",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 4;
    else if (pcrecpp::RE("jun",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 5;
    else if (pcrecpp::RE("jul",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 6;
    else if (pcrecpp::RE("aug",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 7;
    else if (pcrecpp::RE("sep",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 8;
    else if (pcrecpp::RE("oct",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 9;
    else if (pcrecpp::RE("nov",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 10;
    else if (pcrecpp::RE("dec",
        pcrecpp::RE_Options().set_caseless(true)).FullMatch(month))
      time_struct.tm_mon = 11;

    /* set the day of the month and year */
    time_struct.tm_mday = atoi(day.c_str());
    time_struct.tm_year = atoi(year.c_str()) - 1900;

    /* set the hour and minute */
    time_struct.tm_hour = atoi(hour.c_str());
    time_struct.tm_min = atoi(minute.c_str());

    /* set the second if given*/
    if (second.length() > 0)
      time_struct.tm_sec = atoi(second.c_str());

    /* set the day of the week */
    time_struct.tm_wday = getDayOfWeek(time_struct.tm_mon + 1,
                                   time_struct.tm_mday,
                                   time_struct.tm_year + 1900);
  }
  else if (msdos_re.FullMatch(str, &month, &day,
                              &year, &hour, &minute, &am_or_pm))
  {
    /* set the month and the day of the month */
    time_struct.tm_mon = atoi(month.c_str()) - 1;
    time_struct.tm_mday = atoi(day.c_str());

    /* set the year */
    time_struct.tm_year = atoi(year.c_str());
    if (time_struct.tm_year < 70)
      time_struct.tm_year += 100;

    /* set the hour */
    time_struct.tm_hour = atoi(hour.c_str());
    if (time_struct.tm_hour == 12)
      time_struct.tm_hour -= 12;
    if (pcrecpp::RE("PM").FullMatch(am_or_pm))
      time_struct.tm_hour += 12;

    /* set the minute */
    time_struct.tm_min = atoi(minute.c_str());

    /* set the day of the week */
    time_struct.tm_wday = getDayOfWeek(time_struct.tm_mon + 1,
                                   time_struct.tm_mday,
                                   time_struct.tm_year + 1900);
  }

  /* now set m_time */
  m_time = mktime(&time_struct);
}

int CFTPParse::getDayOfWeek(int month, int date, int year)
{
  /* Here we use the Doomsday rule to calculate the day of the week */

  /* First determine the anchor day */
  int anchor;
  if (year >= 1900 && year < 2000)
    anchor = 3;
  else if (year >= 2000 && year < 2100)
    anchor = 2;
  else if (year >= 2100 && year < 2200)
    anchor = 0;
  else if (year >= 2200 && year < 2300)
    anchor = 5;
  else // must have been given an invalid year :-/
    return -1;

  /* Now determine the doomsday */
  int y = year % 100;
  int dday =
    ((y/12 + (y % 12) + ((y % 12)/4)) % 7) + anchor;

  /* Determine if the given year is a leap year */
  int leap_year = 0;
  if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
    leap_year = 1;

  /* Now select a doomsday for the given month */
  int mdday = 1;
  if (month == 1)
  {
    if (leap_year)
      mdday = 4;
    else
      mdday = 3;
  }
  if (month == 2)
  {
    if (leap_year)
      mdday = 1;
    else
      mdday = 7;
  }
  if (month == 3)
    mdday = 7;
  if (month == 4)
    mdday = 4;
  if (month == 5)
    mdday = 9;
  if (month == 6)
    mdday = 6;
  if (month == 7)
    mdday = 11;
  if (month == 8)
    mdday = 8;
  if (month == 9)
    mdday = 5;
  if (month == 10)
    mdday = 10;
  if (month == 11)
    mdday = 9;
  if (month == 12)
    mdday = 12;

  /* Now calculate the day of the week
   * Sunday = 0, Monday = 1, Tuesday = 2, Wednesday = 3, Thursday = 4,
   * Friday = 5, Saturday = 6.
   */
  int day_of_week = ((date - 1) % 7) - ((mdday - 1) % 7) + dday;
  if (day_of_week >= 7)
    day_of_week -= 7;

  return day_of_week;
}

int CFTPParse::FTPParse(string str)
{
  /* Various variable to capture patterns via the regexes */
  string permissions;
  string link_count;
  string owner;
  string group;
  string size;
  string date;
  string name;
  string type;
  string stuff;
  string facts;
  string version;
  string file_id;

  /* Regex for standard Unix listing formats */
  pcrecpp::RE unix_re("^([-bcdlps])" // type
    "([-rwxXsStT]{9})" // permissions
    "\\s+(\\d+)" // hard link count
    "\\s+(\\w+)" // owner
    "\\s+(\\w+)" // group
    "\\s+(\\d+)" // size
    "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // modification date
    "\\s+(.+)$" // name
  );

  /* Regex for NetWare listing formats */
  /* See http://www.novell.com/documentation/oes/ftp_enu/data/a3ep22p.html#fbhbaijf */
  pcrecpp::RE netware_re("^([-d])" // type
    "\\s+(\\[[-SRWCIEMFA]{8}\\])" // rights
    "\\s+(\\w+)" // owner
    "\\s+(\\d+)" // size
    "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // time
    "\\s+(.+)$" // name
  );

  /* Regex for NetPresenz */
  /* See http://files.stairways.com/other/ftp-list-specs-info.txt */
  /* Here we will capture permissions and size if given */
  pcrecpp::RE netpresenz_re("^([-dl])" // type
    "([-rwx]{9}|)" // permissions
    "\\s+(.*)" // stuff
    "\\s+(\\d+|)" // size
    "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // modification date
    "\\s+(.+)$" // name
  );

  /* Regex for EPLF */
  /* See http://cr.yp.to/ftp/list/eplf.html */
  /* SAVE: "(/,|r,|s\\d+,|m\\d+,|i[\\d!#@$%^&*()]+(\\.[\\d!#@$%^&*()]+|),)+" */
  pcrecpp::RE eplf_re("^\\+" // initial "plus" sign
    "([^\\s]+)" // facts
    "\\s(.+)$" // name
  );

  /* Regex for MultiNet */
  /* Best documentation found was
   * http://www-sld.slac.stanford.edu/SLDWWW/workbook/vms_files.html */
  pcrecpp::RE multinet_re("^([^;]+)" // name
    ";(\\d+)" // version
    "\\s+([\\d/]+)" // file id
    "\\s+(\\d{1,2}-[A-Za-z]{3}-\\d{4}\\s+\\d{2}:\\d{2}(:\\d{2})?)" // date
    "\\s+\\[([^\\]]+)\\]" // owner,group
    "\\s+\\(([^\\)]+)\\)$" // permissions
  );

  /* Regex for MSDOS */
  pcrecpp::RE msdos_re("^(\\d{2}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}[AP]M)" // date
    "\\s+(<DIR>|[\\d]+)" // dir or size
    "\\s+(.+)$" // name
  );

  if (unix_re.FullMatch(str, &type, &permissions, &link_count, &owner, &group, &size, &date, &name))
  {
    m_name = name;
    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (pcrecpp::RE("d").FullMatch(type))
      m_flagtrycwd = 1;
    if (pcrecpp::RE("-").FullMatch(type))
      m_flagtryretr = 1;
    if (pcrecpp::RE("l").FullMatch(type))
    {
      m_flagtrycwd = m_flagtryretr = 1;
      // handle symlink
      size_t found = m_name.find(" -> ");
      if (found != std::string::npos)
        m_name = m_name.substr(0, found);
    }
    setTime(date);

    return 1;
  }
  if (netware_re.FullMatch(str, &type, &permissions, &owner, &size, &date, &name))
  {
    m_name = name;
    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (pcrecpp::RE("d").FullMatch(type))
      m_flagtrycwd = 1;
    if (pcrecpp::RE("-").FullMatch(type))
      m_flagtryretr = 1;
    setTime(date);

    return 1;
  }
  if (netpresenz_re.FullMatch(str, &type, &permissions, &stuff, &size, &date, &name))
  {
    m_name = name;
    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (pcrecpp::RE("d").FullMatch(type))
      m_flagtrycwd = 1;
    if (pcrecpp::RE("-").FullMatch(type))
      m_flagtryretr = 1;
    if (pcrecpp::RE("l").FullMatch(type))
    {
      m_flagtrycwd = m_flagtryretr = 1;
      // handle symlink
      size_t found = m_name.find(" -> ");
      if (found != std::string::npos)
        m_name = m_name.substr(0, found);
    }
    setTime(date);

    return 1;
  }
  if (eplf_re.FullMatch(str, &facts, &name))
  {
    /* Get the type, size, and date from the facts */
    pcrecpp::RE("(\\+|,)(r|/),").PartialMatch(facts, (void*)NULL, &type);
    pcrecpp::RE("(\\+|,)s(\\d+),").PartialMatch(facts, (void*)NULL, &size);
    pcrecpp::RE("(\\+|,)m(\\d+),").PartialMatch(facts, (void*)NULL, &date);

    m_name = name;
    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (pcrecpp::RE("/").FullMatch(type))
      m_flagtrycwd = 1;
    if (pcrecpp::RE("r").FullMatch(type))
      m_flagtryretr = 1;
    /* eplf stores the date in time_t format already */
    m_time = atoi(date.c_str());

    return 1;
  }
  if (multinet_re.FullMatch(str, &name, &version, &file_id, &date, (void*)NULL, &owner, &permissions))
  {
    if (pcrecpp::RE("\\.DIR$").PartialMatch(name))
    {
      name.resize(name.size() - 4);
      m_flagtrycwd = 1;
    }
    else
      m_flagtryretr = 1;
    m_name = name;
    setTime(date);
    /* Multinet doesn't provide a size */
    m_size = 0;

    return 1;
  }
  if (msdos_re.FullMatch(str, &date, &size, &name))
  {
    m_name = name;
    if (pcrecpp::RE("<DIR>").FullMatch(size))
    {
      m_flagtrycwd = 1;
      m_size = 0;
    }
    else
    {
      m_flagtryretr = 1;
      m_size = (uint64_t)strtod(size.c_str(), NULL);
    }
    setTime(date);

    return 1;
  }

  return 0;
}

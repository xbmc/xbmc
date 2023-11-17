/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FTPParse.h"

#include <cmath>
#include <regex>

CFTPParse::CFTPParse()
{
  m_flagtrycwd = 0;
  m_flagtryretr = 0;
  m_size = 0;
}

std::string CFTPParse::getName()
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

namespace
{
  const std::string months = "janfebmaraprmayjunjulaugsepoctnovdec";

  // set time_struct.tm_mon from the 3-letter "abbreviated month name"
  void setMonFromName(struct tm& time_struct, const std::string& name)
  {
    std::smatch match;
    if (std::regex_search(months, match, std::regex(name, std::regex_constants::icase))) 
    {
        int pos = match.position();
        if (name.size() == 3 && pos % 3 == 0)
            time_struct.tm_mon = pos / 3;
    }
  }
}

void CFTPParse::setTime(const std::string& str)
{
  /* Variables used to capture patterns via the regexes */
  std::string month;
  std::string day;
  std::string year;
  std::string hour;
  std::string minute;
  std::string second;
  std::string am_or_pm;
  std::smatch match;

  /* time struct used to set the time_t variable */
  struct tm time_struct = {};

  /* Regex to read Unix, NetWare and NetPresenz time format */
  std::regex unix_re("^([A-Za-z]{3})" // month
    "\\s+(\\d{1,2})" // day of month
    "\\s+([:\\d]{4,5})$" // time of day or year
  );

  /* Regex to read MultiNet time format */
  std::regex multinet_re("^(\\d{1,2})" // day of month
    "-([A-Za-z]{3})" // month
    "-(\\d{4})" // year
    "\\s+(\\d{2})" // hour
    ":(\\d{2})" // minute
    "(:(\\d{2}))?$" // second
  );

  /* Regex to read MSDOS time format */
  std::regex msdos_re("^(\\d{2})" // month
    "-(\\d{2})" // day of month
    "-(\\d{2})" // year
    "\\s+(\\d{2})" // hour
    ":(\\d{2})" // minute
    "([AP]M)$" // AM or PM
  );

  if (std::regex_match(str, match, unix_re))
  {
    month = match[1].str();
    day = match[2].str();
    year = match[3].str();

    /* set the month */
    setMonFromName(time_struct, month);

    /* set the day of the month */
    time_struct.tm_mday = atoi(day.c_str());

    time_t t = time(NULL);
    struct tm *current_time;
#ifdef LOCALTIME_R
    struct tm result = {};
    current_time = localtime_r(&t, &result);
#else
    current_time = localtime(&t);
#endif
    if (std::regex_match(year, match, std::regex("(\\d{2}):(\\d{2})")))
    {
      /* set the hour and minute */
      time_struct.tm_hour = atoi(match[1].str().c_str());
      time_struct.tm_min = atoi(match[2].str().c_str());

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
  else if (std::regex_match(str, match, multinet_re))
  {
    day = match[1].str();
    month = match[2].str();
    year = match[3].str();
    hour = match[4].str();
    minute = match[5].str();
    // match[6] ignored
    second = match[7].str();

    /* set the month */
    setMonFromName(time_struct, month);

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
  else if (std::regex_match(str, match, msdos_re))
  {
    month = match[1].str();
    day = match[2].str();
    year = match[3].str();
    hour = match[4].str();
    minute = match[5].str();
    am_or_pm = match[6].str();

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
    if (am_or_pm == "PM")
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

int CFTPParse::FTPParse(const std::string& str)
{
  /* Various variable to capture patterns via the regexes */
  std::string permissions;
  std::string link_count;
  std::string owner;
  std::string group;
  std::string size;
  std::string date;
  std::string name;
  std::string type;
  std::string stuff;
  std::string facts;
  std::string version;
  std::string file_id;
  std::smatch match;

  /* Regex for standard Unix listing formats */
  std::regex unix_re("^([-bcdlps])" // type
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
  std::regex netware_re("^([-d])" // type
    "\\s+(\\[[-SRWCIEMFA]{8}\\])" // rights
    "\\s+(\\w+)" // owner
    "\\s+(\\d+)" // size
    "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // time
    "\\s+(.+)$" // name
  );

  /* Regex for NetPresenz */
  /* See http://files.stairways.com/other/ftp-list-specs-info.txt */
  /* Here we will capture permissions and size if given */
  std::regex netpresenz_re("^([-dl])" // type
    "([-rwx]{9}|)" // permissions
    "\\s+(.*)" // stuff
    "\\s+(\\d+|)" // size
    "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // modification date
    "\\s+(.+)$" // name
  );

  /* Regex for EPLF */
  /* See http://cr.yp.to/ftp/list/eplf.html */
  /* SAVE: "(/,|r,|s\\d+,|m\\d+,|i[\\d!#@$%^&*()]+(\\.[\\d!#@$%^&*()]+|),)+" */
  std::regex eplf_re("^\\+" // initial "plus" sign
    "([^\\s]+)" // facts
    "\\s(.+)$" // name
  );

  /* Regex for MultiNet */
  /* Best documentation found was
   * http://www-sld.slac.stanford.edu/SLDWWW/workbook/vms_files.html */
  std::regex multinet_re("^([^;]+)" // name
    ";(\\d+)" // version
    "\\s+([\\d/]+)" // file id
    "\\s+(\\d{1,2}-[A-Za-z]{3}-\\d{4}\\s+\\d{2}:\\d{2}(:\\d{2})?)" // date
    "\\s+\\[([^\\]]+)\\]" // owner,group
    "\\s+\\(([^\\)]+)\\)$" // permissions
  );

  /* Regex for MSDOS */
  std::regex msdos_re("^(\\d{2}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}[AP]M)" // date
    "\\s+(<DIR>|[\\d]+)" // dir or size
    "\\s+(.+)$" // name
  );

  if (std::regex_match(str, match, unix_re))
  {
    type = match[1].str();
    permissions = match[2].str();
    link_count = match[3].str();
    owner = match[4].str();
    group = match[5].str();
    size = match[6].str();
    date = match[7].str();
    m_name = match[8].str();

    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (type == "d")
      m_flagtrycwd = 1;
    if (type == "-")
      m_flagtryretr = 1;
    if (type == "l")
    {
      m_flagtrycwd = m_flagtryretr = 1;
      // handle symlink
      size_t found = m_name.find(" -> ");
      if (found != std::string::npos)
        m_name.resize(found);
    }
    setTime(date);

    return 1;
  }
  if (std::regex_match(str, match, netware_re))
  {
    type = match[1].str();
    permissions = match[2].str();
    owner = match[3].str();
    size = match[4].str();
    date = match[5].str();
    m_name = match[6].str();

    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (type == "d")
      m_flagtrycwd = 1;
    if (type == "-")
      m_flagtryretr = 1;
    setTime(date);

    return 1;
  }
  if (std::regex_match(str, match, netpresenz_re))
  {
    type = match[1].str();
    permissions = match[2].str();
    stuff = match[3].str();
    size = match[4].str();
    date = match[5].str();
    m_name = match[6].str();

    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (type == "d")
      m_flagtrycwd = 1;
    if (type == "-")
      m_flagtryretr = 1;
    if (type == "l")
    {
      m_flagtrycwd = m_flagtryretr = 1;
      // handle symlink
      size_t found = m_name.find(" -> ");
      if (found != std::string::npos)
        m_name.resize(found);
    }
    setTime(date);

    return 1;
  }
  if (std::regex_match(str, match, eplf_re))
  {
    facts = match[1].str();
    m_name = match[2].str();

    /* Get the type, size, and date from the facts */
    std::regex_search(facts, match, std::regex("(?:\\+|,)(r|/),"));
    type = match[1].str();
    std::regex_search(facts, match, std::regex("(?:\\+|,)s(\\d+),"));
    size = match[1].str();
    std::regex_search(facts, match, std::regex("(?:\\+|,)m(\\d+),"));
    date = match[1].str();

    m_size = (uint64_t)strtod(size.c_str(), NULL);
    if (type == "/")
      m_flagtrycwd = 1;
    if (type == "r")
      m_flagtryretr = 1;
    /* eplf stores the date in time_t format already */
    m_time = atoi(date.c_str());

    return 1;
  }
  if (std::regex_match(str, match, multinet_re))
  {
    name = match[1].str();
    version = match[2].str();
    file_id = match[3].str();
    date = match[4].str();
    // match[5] ignored 
    owner = match[6].str();
    permissions = match[7].str();

    if (std::regex_search(name, std::regex("\\.DIR$")))
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
  if (std::regex_match(str, match, msdos_re))
  {
    date = match[1].str();
    size = match[2].str();
    m_name = match[3].str();
    if (size == "<DIR>")
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

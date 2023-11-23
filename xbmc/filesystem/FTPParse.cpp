/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FTPParse.h"

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
} // namespace

void CFTPParse::setTime(const std::string& str)
{
  /* time struct used to set the time_t variable */
  struct tm time_struct = {};

  // sub matches from regex_match() calls below
  std::smatch match;

  // Regex to read Unix, NetWare and NetPresenz time format
  if (std::regex_match(str, match,
                       std::regex("^([A-Za-z]{3})" // month
                                  "\\s+(\\d{1,2})" // day of month
                                  "\\s+([:\\d]{4,5})$"))) // time of day or year
  {
    auto month = match[1].str();
    auto day = match[2].str();
    auto year = match[3].str();

    /* set the month */
    setMonFromName(time_struct, month);

    /* set the day of the month */
    time_struct.tm_mday = std::stol(day);

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
      time_struct.tm_hour = std::stol(match[1].str());
      time_struct.tm_min = std::stol(match[2].str());

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
      time_struct.tm_year = std::stol(year) - 1900;
    }
  }
  // Regex to read MultiNet time format
  else if (std::regex_match(str, match,
                            std::regex("^(\\d{1,2})" // day of month
                                       "-([A-Za-z]{3})" // month
                                       "-(\\d{4})" // year
                                       "\\s+(\\d{2})" // hour
                                       ":(\\d{2})" // minute
                                       "(:(\\d{2}))?$"))) // second
  {
    auto day = match[1].str();
    auto month = match[2].str();
    auto year = match[3].str();
    auto hour = match[4].str();
    auto minute = match[5].str();
    // match[6] ignored
    auto second = match[7].str();

    /* set the month */
    setMonFromName(time_struct, month);

    /* set the day of the month and year */
    time_struct.tm_mday = std::stol(day);
    time_struct.tm_year = std::stol(year) - 1900;

    /* set the hour and minute */
    time_struct.tm_hour = std::stol(hour);
    time_struct.tm_min = std::stol(minute);

    /* set the second if given*/
    if (second.length() > 0)
      time_struct.tm_sec = std::stol(second);
  }
  // Regex to read MSDOS time format
  else if (std::regex_match(str, match,
                            std::regex("^(\\d{2})" // month
                                       "-(\\d{2})" // day of month
                                       "-(\\d{2})" // year
                                       "\\s+(\\d{2})" // hour
                                       ":(\\d{2})" // minute
                                       "([AP]M)$"))) // AM or PM
  {
    auto month = match[1].str();
    auto day = match[2].str();
    auto year = match[3].str();
    auto hour = match[4].str();
    auto minute = match[5].str();
    auto am_or_pm = match[6].str();

    /* set the month and the day of the month */
    time_struct.tm_mon = std::stol(month) - 1;
    time_struct.tm_mday = std::stol(day);

    /* set the year */
    time_struct.tm_year = std::stoi(year);
    if (time_struct.tm_year < 70)
      time_struct.tm_year += 100;

    /* set the hour */
    time_struct.tm_hour = std::stoi(hour);
    if (time_struct.tm_hour == 12)
      time_struct.tm_hour -= 12;
    if (am_or_pm == "PM")
      time_struct.tm_hour += 12;

    /* set the minute */
    time_struct.tm_min = std::stoi(minute);
  }

  /* now set m_time */
  m_time = mktime(&time_struct);
}

int CFTPParse::FTPParse(const std::string& str)
{
  // sub matches from regex_match() calls below
  std::smatch match;

  // Regex for standard Unix listing formats
  if (std::regex_match(
          str, match,
          std::regex("^([-bcdlps])" // type
                     "([-rwxXsStT]{9})" // permissions
                     "\\s+(\\d+)" // hard link count
                     "\\s+(\\w+)" // owner
                     "\\s+(\\w+)" // group
                     "\\s+(\\d+)" // size
                     "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // modification date
                     "\\s+(.+)$"))) // name
  {
    auto type = match[1].str();
    auto permissions = match[2].str();
    auto link_count = match[3].str();
    auto owner = match[4].str();
    auto group = match[5].str();
    auto size = match[6].str();
    auto date = match[7].str();
    m_name = match[8].str();

    m_size = std::stoull(size);
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
  // Regex for NetWare listing formats
  // See http://www.novell.com/documentation/oes/ftp_enu/data/a3ep22p.html#fbhbaijf
  if (std::regex_match(str, match,
                       std::regex("^([-d])" // type
                                  "\\s+(\\[[-SRWCIEMFA]{8}\\])" // rights
                                  "\\s+(\\w+)" // owner
                                  "\\s+(\\d+)" // size
                                  "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // time
                                  "\\s+(.+)$"))) // name
  {
    auto type = match[1].str();
    auto permissions = match[2].str();
    auto owner = match[3].str();
    auto size = match[4].str();
    auto date = match[5].str();
    m_name = match[6].str();

    m_size = std::stoull(size);
    if (type == "d")
      m_flagtrycwd = 1;
    if (type == "-")
      m_flagtryretr = 1;
    setTime(date);

    return 1;
  }
  // Regex for NetPresenz
  // See http://files.stairways.com/other/ftp-list-specs-info.txt
  // Here we will capture permissions and size if given
  if (std::regex_match(
          str, match,
          std::regex("^([-dl])" // type
                     "([-rwx]{9}|)" // permissions
                     "\\s+(.*)" // stuff
                     "\\s+(\\d+|)" // size
                     "\\s+([A-Za-z]{3}\\s+\\d{1,2}\\s+[:\\d]{4,5})" // modification date
                     "\\s+(.+)$"))) // name
  {
    auto type = match[1].str();
    auto permissions = match[2].str();
    auto stuff = match[3].str();
    auto size = match[4].str();
    auto date = match[5].str();
    m_name = match[6].str();

    m_size = std::stoull(size);
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
  // Regex for EPLF
  // See http://cr.yp.to/ftp/list/eplf.html
  // SAVE: "(/,|r,|s\\d+,|m\\d+,|i[\\d!#@$%^&*()]+(\\.[\\d!#@$%^&*()]+|),)+"
  if (std::regex_match(str, match,
                       std::regex("^\\+" // initial "plus" sign
                                  "([^\\s]+)" // facts
                                  "\\s(.+)$"))) // name
  {
    auto facts = match[1].str();
    m_name = match[2].str();

    /* Get the type, size, and date from the facts */
    std::regex_search(facts, match, std::regex("(?:\\+|,)(r|/),"));
    auto type = match[1].str();
    std::regex_search(facts, match, std::regex("(?:\\+|,)s(\\d+),"));
    auto size = match[1].str();
    std::regex_search(facts, match, std::regex("(?:\\+|,)m(\\d+),"));
    auto date = match[1].str();

    m_size = std::stoull(size);
    if (type == "/")
      m_flagtrycwd = 1;
    if (type == "r")
      m_flagtryretr = 1;
    /* eplf stores the date in time_t format already */
    m_time = std::stoi(date);

    return 1;
  }
  // Regex for MultiNet
  // Best documentation found was
  // http://www-sld.slac.stanford.edu/SLDWWW/workbook/vms_files.html
  if (std::regex_match(
          str, match,
          std::regex("^([^;]+)" // name
                     ";(\\d+)" // version
                     "\\s+([\\d/]+)" // file id
                     "\\s+(\\d{1,2}-[A-Za-z]{3}-\\d{4}\\s+\\d{2}:\\d{2}(:\\d{2})?)" // date
                     "\\s+\\[([^\\]]+)\\]" // owner,group
                     "\\s+\\(([^\\)]+)\\)$"))) // permissions
  {
    auto name = match[1].str();
    auto version = match[2].str();
    auto file_id = match[3].str();
    auto date = match[4].str();
    // match[5] ignored
    auto owner = match[6].str();
    auto permissions = match[7].str();

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
  // Regex for MSDOS
  if (std::regex_match(str, match,
                       std::regex("^(\\d{2}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}[AP]M)" // date
                                  "\\s+(<DIR>|[\\d]+)" // dir or size
                                  "\\s+(.+)$"))) // name
  {
    auto date = match[1].str();
    auto size = match[2].str();
    m_name = match[3].str();
    if (size == "<DIR>")
    {
      m_flagtrycwd = 1;
      m_size = 0;
    }
    else
    {
      m_flagtryretr = 1;
      m_size = std::stoull(size);
    }
    setTime(date);

    return 1;
  }

  return 0;
}

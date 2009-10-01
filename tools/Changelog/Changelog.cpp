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

// Changelog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "tinyxml.h"
#include "../../xbmc/utils/RegExp.h"

#ifdef _LINUX
#define _tmain main
#define _TCHAR char
#endif

const char header[] = "*************************************************************************************************************\r\n"
                      "*************************************************************************************************************\r\n"
                      "                                     XBMC CHANGELOG\r\n"
                      "*************************************************************************************************************\r\n"
                      "*************************************************************************************************************\r\n"
                      "\r\n"
                      "Date        Rev   Message\r\n"
                      "=============================================================================================================\r\n";

const char filter[][100] = {"[- ]*[0-9]+-[0-9]+-[0-9]+ *",
                             "\\*\\*\\* empty log message \\*\\*\\*",
                             "no message" };

std::string FilterMessage(std::string message)
{
  std::string filteredMessage = message;
  CRegExp reg;
  for (int i = 0; i < sizeof(filter) / 100; i++)
  {
    reg.RegComp(filter[i]);
    int findStart = reg.RegFind(message.c_str());
    while (findStart >= 0)
    {
      filteredMessage = message.substr(0, findStart);
      filteredMessage.append(message.substr(findStart + reg.GetFindLen(), message.length()));
      message = filteredMessage;
      findStart = reg.RegFind(message.c_str());
    }
  }
  return filteredMessage;
}

int _tmain(int argc, _TCHAR* argv[])
{
  std::string input = "svn_log.xml";
  std::string output = "Changelog.txt";
  int limit = 0;

  if (argc < 2)
  {
    // output help information
    printf("usage:\n");
    printf("\n");
    printf("  Changelog input <output> <limit>\n");
    printf("\n");
    printf("  input    : input .xml file generated from SVN (using svn log --xml)\n");
    printf("             DOWNLOAD to download direct from XBMC SVN\n");
    printf("  <output> : output .txt file for the changelog (defaults to Changelog.txt)\n");
    printf("  <limit>  : the number of log entries for svn to fetch. (defaults to no limit)");
    printf("\n");
    return 0;
  }
  input = argv[1];
  if (argc > 2)
    output = argv[2];
  FILE *file = fopen(output.c_str(), "wb");
  if (!file)
    return 1;
  fprintf(file, header);
  if (input.compare("download") == 0)
  {
    if(argc > 3)
      limit = atoi(argv[3]);
    // download our input file
    std::string command = "svn log -r 'HEAD':8638 ";
    if (limit > 0)
    {
      command += "--limit ";
      command += argv[3]; // the limit as a string
      command += " ";
    }
#ifndef _LINUX
    command += "--xml https://xbmc.svn.sourceforge.net/svnroot/xbmc/trunk/XBMC > svn_log.xml";
#else
    command += "--xml https://xbmc.svn.sourceforge.net/svnroot/xbmc/branches/linuxport/XBMC > svn_log.xml";
#endif
    printf("Downloading changelog from SVN - this will take some time (around 1MB to download with no limit)\n");
    system(command.c_str());
    input = "svn_log.xml";
    printf("Downloading done - processing\n");
  }
  TiXmlDocument doc;
  if (!doc.LoadFile(input.c_str()))
  {
    return 1;
  }

  TiXmlElement *root = doc.RootElement();
  if (!root) return 1;

  TiXmlElement *logitem = root->FirstChildElement("logentry");
  while (logitem)
  {
    int revision;
    logitem->Attribute("revision", &revision);
    TiXmlNode *date = logitem->FirstChild("date");
    std::string dateString;
    if (date && date->FirstChild())
      dateString = date->FirstChild()->Value();
    TiXmlNode *msg = logitem->FirstChild("msg");
    if (msg && msg->FirstChild())
    {
      // filter the message a bit
      std::string message = FilterMessage(msg->FirstChild()->Value());
      if (message.size())
        fprintf(file, "%s  %4i  %s\r\n", dateString.substr(0,10).c_str(), revision, message.c_str());
      else
        int breakhere = 1;
    }
    logitem = logitem->NextSiblingElement("logentry");
  }
  fclose(file);
  printf("Changelog saved as: %s\n", output.c_str());
  return 0;
}

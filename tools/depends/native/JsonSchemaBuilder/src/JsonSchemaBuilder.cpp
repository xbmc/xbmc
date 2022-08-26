/*
 *      Copyright (C) 2011 Tobias Arrskog
 *      https://github.com/topfs2/jsd_builder
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

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

void print_version(std::ifstream &in, std::ofstream &out)
{
  std::string line;
  if (getline(in, line))
    out << std::regex_replace(line, std::regex("(\\s+)?JSONRPC_VERSION\\s+|(\\s+)?#.*"), "");
}

void print_license(std::ifstream &in, std::ofstream &out)
{
  std::string line;

  while (getline(in, line, '\n'))
    out << line << std::endl;
}

void print_json(std::ifstream &in, std::ofstream &out)
{
  std::string line;
  unsigned int count = 0;
  bool closing = false;

  while (getline(in, line, '\n'))
  {
    // No need to handle the last line
    if (line == "}")
    {
      out << std::endl;
      continue;
    }

    // If we just closed a whole object we need to print the separator
    if (closing)
      out << "," << std::endl;

    out << "  ";
    bool started = false;
    closing = false;
    for (std::string::iterator itr = line.begin(); itr != line.end(); ++itr)
    {
      // Skip \r characters
      if (*itr == '\r') {
        break;
      }

      // Count opening { but ignore the first one
      if (*itr == '{')
      {
        count++;
        if (count == 1)
          break;
      }
      // Replace tabs with 2 spaces
      if (*itr == '\t')
      {
        out << "  ";
        continue;
      }
      // Count closing } but ignore the last one
      if (*itr == '}')
      {
        count--;
        if (count == 0)
          break;

        if (count == 1)
        {
          out << "\"}\"";
          closing = true;
          break;
        }
      }
      // Only print a " before the first real sign
      if (!started && *itr != ' ')
      {
        started = true;
        out << '"';
      }
      // Add a backslash before a double-quote and backslashes
      if (*itr == '"' || *itr == '\\')
        out << '\\';
      out << (*itr);
    }
    // Only print a closing " if there was real content on the line
    if (started)
      out << '"';

    // Only print a newline if we haven't just closed a whole object
    if (!closing)
      out << std::endl;
  }
}

void print_usage(const char *application)
{
  std::cout << application << " version.txt license.txt methods.json types.json notifications.json" << std::endl;
}

int main(int argc, char* argv[])
{
  if (argc < 6)
  {
    print_usage(argv[0]);
    return -1;
  }

  std::ofstream out ("ServiceDescription.h", std::ofstream::binary);

  std::ifstream version(argv[1], std::ios_base::in);
  std::ifstream license(argv[2], std::ios_base::in);
  std::ifstream methods(argv[3], std::ios_base::in);
  std::ifstream types(argv[4], std::ios_base::in);
  std::ifstream notifications(argv[5], std::ios_base::in);

  if (!(version && license && methods && types && notifications))
  {
    std::cout << "Failed to find one or more of version.txt, license.txt, methods.json, types.json or notifications.json" << std::endl;
    return -1;
  }

  out << "#pragma once" << std::endl;

  print_license(license, out);

  out << std::endl;

  out << "namespace JSONRPC" << std::endl;
  out << "{" << std::endl;
  out << "  const char* const JSONRPC_SERVICE_ID          = \"http://xbmc.org/jsonrpc/ServiceDescription.json\";" << std::endl;
  out << "  const char* const JSONRPC_SERVICE_VERSION     = \""; print_version(version, out); out << "\";" << std::endl;
  out << "  const char* const JSONRPC_SERVICE_DESCRIPTION = \"JSON-RPC API of XBMC\";" << std::endl;
  out << std::endl;

  out << "  const char* const JSONRPC_SERVICE_TYPES[] = {";
  print_json(types, out);
  out << "  };" << std::endl;
  out << std::endl;

  out << "  const char* const JSONRPC_SERVICE_METHODS[] = {";
  print_json(methods, out);
  out << "  };" << std::endl;
  out << std::endl;

  out << "  const char* const JSONRPC_SERVICE_NOTIFICATIONS[] = {";
  print_json(notifications, out);
  out << "  };" << std::endl;

  out << "}" << std::endl;

  return 0;
}

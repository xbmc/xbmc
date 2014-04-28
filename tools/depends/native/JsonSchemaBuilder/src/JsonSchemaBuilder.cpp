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
#include <string>

using namespace std;

void print_version(ifstream &in, ofstream &out)
{
  string line;
  if (getline(in, line))
    out << line;
}

void print_license(ifstream &in, ofstream &out)
{
  string line;

  while (getline(in, line, '\n'))
    out << line << endl;
}

void print_json(ifstream &in, ofstream &out)
{
  string line;
  unsigned int count = 0;
  bool closing = false;

  while (getline(in, line, '\n'))
  {
    // No need to handle the last line
    if (line == "}")
    {
      out << endl;
      continue;
    }
    
    // If we just closed a whole object we need to print the separator
    if (closing)
      out << "," << endl;
    
    out << "  ";
    bool started = false;
    closing = false;
    for (string::iterator itr = line.begin(); itr != line.end(); itr++)
    {
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
      out << endl;
  }
}

void print_usage(const char *application)
{
  cout << application << " version.txt license.txt methods.json types.json notifications.json" << endl;
}

int main(int argc, char* argv[])
{
  if (argc < 6)
  {
    print_usage(argv[0]);
    return -1;
  }

  ofstream out ("ServiceDescription.h", ofstream::binary);

  ifstream version(argv[1], ios_base::in);
  ifstream license(argv[2], ios_base::in);
  ifstream methods(argv[3], ios_base::in);
  ifstream types(argv[4], ios_base::in);
  ifstream notifications(argv[5], ios_base::in);

  if (!(version && license && methods && types && notifications))
  {
    cout << "Failed to find one or more of version.txt, license.txt, methods.json, types.json or notifications.json" << endl;
    return -1;
  }

  out << "#pragma once" << endl;

  print_license(license, out);

  out << endl;

  out << "namespace JSONRPC" << endl;
  out << "{" << endl;
  out << "  const char* const JSONRPC_SERVICE_ID          = \"http://xbmc.org/jsonrpc/ServiceDescription.json\";" << endl;
  out << "  const char* const JSONRPC_SERVICE_VERSION     = \""; print_version(version, out); out << "\";" << endl;
  out << "  const char* const JSONRPC_SERVICE_DESCRIPTION = \"JSON-RPC API of XBMC\";" << endl;
  out << endl;

  out << "  const char* const JSONRPC_SERVICE_TYPES[] = {";
  print_json(types, out);
  out << "  };" << endl;
  out << endl;

  out << "  const char* const JSONRPC_SERVICE_METHODS[] = {";
  print_json(methods, out);
  out << "  };" << endl;
  out << endl;

  out << "  const char* const JSONRPC_SERVICE_NOTIFICATIONS[] = {";
  print_json(notifications, out);
  out << "  };" << endl;

  out << "}" << endl;

  return 0;
}

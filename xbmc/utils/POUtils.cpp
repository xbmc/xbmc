/*
 *      Copyright (C) 2012 Team XBMC
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

#include "utils/POUtils.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include <stdlib.h>

CPODocument::CPODocument()
{
  m_CursorPos = 0;
  m_nextEntryPos = 0;
  m_POfilelength = 0;
  m_Entry.msgStrPlural.resize(1);
}

CPODocument::~CPODocument() {}

bool CPODocument::LoadFile(const std::string &pofilename)
{
  XFILE::CFile file;
  if (!file.Open(pofilename))
    return false;

  m_POfilelength = static_cast<size_t> (file.GetLength());
  if (m_POfilelength < 1)
  {
    file.Close();
    CLog::Log(LOGERROR, "POParser: zero length found for string file: %s", pofilename.c_str());
    return false;
  }

  m_strBuffer.resize(m_POfilelength+1);

  unsigned int readBytes = file.Read(&m_strBuffer[1], m_POfilelength);
  file.Close();

  if (readBytes != m_POfilelength)
  {
    CLog::Log(LOGERROR, "POParser: actual read data differs from filesize, for string file: %s",
              pofilename.c_str());
    return false;
  }

  // we make sure, to have an LF at beginning and at end of buffer
  m_strBuffer[0] = '\n';
  if (*m_strBuffer.rbegin() != '\n')
  {
    m_strBuffer += "\n";
    m_POfilelength++;
  }

  if (GetNextEntry() && m_Entry.Type == MSGID_FOUND)
    return true;

  CLog::Log(LOGERROR, "POParser: unable to read PO file header from file: %s", pofilename.c_str());
  return false;
}

bool CPODocument::GetNextEntry()
{
  do
  {
    // if condition is true we reached the end of buffer AND we have not found a valid entry
    if (m_nextEntryPos == m_POfilelength)
      return false;

    // if we don't find "\n\n", we reached the end of the buffer and the last entry to check
    if ((m_nextEntryPos = m_strBuffer.find("\n\n", m_CursorPos)) == std::string::npos)
      m_nextEntryPos = m_POfilelength;

    // now we read the actual entry into a temp string for further processing
    // string constructor + swap is 20% faster than substr
    std::string strEntry (m_strBuffer, m_CursorPos, m_nextEntryPos - m_CursorPos +1);
    m_Entry.Content.swap(strEntry);

    m_CursorPos = m_nextEntryPos+1; // jump cursor to the second \n character
  }

  while (!FindLineStart ("msgid ", m_Entry.msgID.Pos));
  // while goes on, until we find a valid entry

  if (FindLineStart ("#: id:", m_Entry.xIDPos) && ParseNumID())
  {
      m_Entry.Type = ID_FOUND; // we found an entry with a valid numeric id
      return true;
  }

  size_t plurPos;
  if (FindLineStart ("msgid_plural ", plurPos))
  {
    m_Entry.Type = MSGID_PLURAL_FOUND; // we found a pluralized entry
    return true;
  }

  m_Entry.Type = MSGID_FOUND; // we found a normal entry, with no numeric id
  return true;
}

void CPODocument::ParseEntry(bool bisSourceLang)
{
  if (bisSourceLang)
  {
    if (m_Entry.Type == ID_FOUND)
      GetString(m_Entry.msgID);
    else
      m_Entry.msgID.Str.clear();
    return;
  }

  if (!m_Entry.Type == ID_FOUND)
  {
    GetString(m_Entry.msgID);
    if (FindLineStart ("msgctxt ", m_Entry.msgCtxt.Pos))
      GetString(m_Entry.msgCtxt);
    else
      m_Entry.msgCtxt.Str.clear();
  }

  if (m_Entry.Type != MSGID_PLURAL_FOUND)
  {
    if (FindLineStart ("msgstr ", m_Entry.msgStr.Pos))
      GetString(m_Entry.msgStr);
    else
    {
      CLog::Log(LOGERROR, "POParser: missing msgstr line in entry. Failed entry: %s",
                m_Entry.Content.c_str());
      m_Entry.msgStr.Str.clear();
    }
    return;
  }

  m_Entry.msgStrPlural.resize(1);
  m_Entry.msgStrPlural[0].Str.clear();
  size_t plurPos;
  std::string strPattern = "msgstr[0] ";

  for (int n=0; n<7 ; n++)
  {
    strPattern[7] = static_cast<char>(n+'0');
    if (FindLineStart (strPattern, plurPos))
    {
      m_Entry.msgStrPlural.resize(n+1);
      m_Entry.msgStrPlural[n].Pos = plurPos;
      GetString(m_Entry.msgStrPlural[n]);
      if (m_Entry.msgStrPlural[n].Str.empty())
        break;
    }
    else
      break;
  }

  return;
}

std::string* CPODocument::GetPlurMsgstr(size_t plural)
{
  if ((m_Entry.msgStrPlural.size()) < plural+1)
  {
    CLog::Log(LOGERROR, "POParser: msgstr[%i] plural field requested, but not found in PO file. "
              "Failed entry: %s",
              static_cast<int>(plural), m_Entry.Content.c_str());
    plural = m_Entry.msgStrPlural.size()-1;
  }
  return &m_Entry.msgStrPlural[plural].Str;
}

std::string CPODocument::UnescapeString(std::string &strInput)
{
  char oescchar;
  std::string strOutput;
  std::string::const_iterator it = strInput.begin();
  while (it < strInput.end())
  {
    oescchar = *it++;
    if (oescchar == '\\')
    {
      if (it == strInput.end())
      {
        CLog::Log(LOGERROR,
                  "POParser: warning, unhandled escape character "
                  "at line-end. Problematic entry: %s",
                  m_Entry.Content.c_str());
        continue;
      }
      switch (*it++)
      {
        case 'a':  oescchar = '\a'; break;
        case 'b':  oescchar = '\b'; break;
        case 'v':  oescchar = '\v'; break;
        case 'n':  oescchar = '\n'; break;
        case 't':  oescchar = '\t'; break;
        case 'r':  oescchar = '\r'; break;
        case '"':  oescchar = '"' ; break;
        case '0':  oescchar = '\0'; break;
        case 'f':  oescchar = '\f'; break;
        case '?':  oescchar = '\?'; break;
        case '\'': oescchar = '\''; break;
        case '\\': oescchar = '\\'; break;

        default: 
        {
          CLog::Log(LOGERROR,
                    "POParser: warning, unhandled escape character. Problematic entry: %s",
                    m_Entry.Content.c_str());
          continue;
        }
      }
    }
    strOutput += oescchar;
  }
  return strOutput;
}

bool CPODocument::FindLineStart(std::string strToFind, size_t &FoundPos)
{
  strToFind.insert(0, "\n");

  FoundPos = m_Entry.Content.find(strToFind.c_str());

  if (FoundPos == std::string::npos || strToFind.size() > m_Entry.Content.size() - FoundPos -2)
    return false; // if we don't find the string or if we don't have at least one char after it

  FoundPos += strToFind.size(); // to set the pos marker to the exact start of the real data
  return true;
}

bool CPODocument::ParseNumID()
{
  if (isdigit(m_Entry.Content.at(m_Entry.xIDPos))) // verify if the first char is digit
  {
    // we check for the numeric id for the fist 10 chars (uint32)
    m_Entry.xID = strtol(m_Entry.Content.substr(m_Entry.xIDPos,10).c_str(), NULL, 10);
    return true;
  }

  CLog::Log(LOGERROR, "POParser: found numeric id descriptor, but no valid id can be read, "
            "entry was handled as normal msgid entry");
  CLog::Log(LOGERROR, "POParser: The problematic entry: %s",
            m_Entry.Content.c_str());
  return false;
}

void CPODocument::GetString(CStrEntry &strEntry)
{
  size_t nextLFPos;
  size_t startPos = strEntry.Pos;
  strEntry.Str.clear();

  do
  {
    nextLFPos = m_Entry.Content.find("\n", startPos);
    if (nextLFPos == std::string::npos)
      nextLFPos = m_Entry.Content.size();

    // check syntax, if it really is a valid quoted stringline
    if (m_Entry.Content[startPos] != '\"' || m_Entry.Content[nextLFPos-1] != '\"')
      break;

    strEntry.Str.append(m_Entry.Content, startPos+1, nextLFPos-2-startPos);
    startPos = nextLFPos+1;
  }
  while (startPos < m_Entry.Content.size());

  strEntry.Str = UnescapeString(strEntry.Str);
}

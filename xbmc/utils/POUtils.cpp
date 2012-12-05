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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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
  m_Entry.msgStrPlural.clear();
  m_Entry.msgStrPlural.resize(1);
}

CPODocument::~CPODocument() {}

bool CPODocument::LoadFile(const std::string &pofilename)
{
  XFILE::CFile file;
  if (!file.Open(pofilename))
    return false;

  int64_t fileLength = file.GetLength();
  if (fileLength < 18) // at least a size of a minimalistic header
  {
    file.Close();
    CLog::Log(LOGERROR, "POParser: non valid length found for string file: %s", pofilename.c_str());
    return false;
  }

  m_POfilelength = static_cast<size_t> (fileLength);

  m_strBuffer.resize(m_POfilelength+1);
  m_strBuffer[0] = '\n';

  unsigned int readBytes = file.Read(&m_strBuffer[1], m_POfilelength);
  file.Close();

  if (readBytes != m_POfilelength)
  {
    CLog::Log(LOGERROR, "POParser: actual read data differs from file size, for string file: %s",
              pofilename.c_str());
    return false;
  }

  ConvertLineEnds(pofilename);

  // we make sure, to have an LF at the end of buffer
  if (*m_strBuffer.rbegin() != '\n')
  {
    m_strBuffer += "\n";
  }

  m_POfilelength = m_strBuffer.size();

  if (GetNextEntry() && m_Entry.Type == MSGID_FOUND)
    return true;

  CLog::Log(LOGERROR, "POParser: unable to read PO file header from file: %s", pofilename.c_str());
  return false;
}

bool CPODocument::GetNextEntry()
{
  do
  {
    // if we don't find LFLF, we reached the end of the buffer and the last entry to check
    // we indicate this with setting m_nextEntryPos to the end of the buffer
    if ((m_nextEntryPos = m_strBuffer.find("\n\n", m_CursorPos)) == std::string::npos)
      m_nextEntryPos = m_POfilelength-1;

    // now we read the actual entry into a temp string for further processing
    m_Entry.Content.assign(m_strBuffer, m_CursorPos, m_nextEntryPos - m_CursorPos +1);
    m_CursorPos = m_nextEntryPos+1; // jump cursor to the second LF character

    if (FindLineStart ("\nmsgid ", m_Entry.msgID.Pos))
    {
      if (FindLineStart ("\nmsgctxt \"#", m_Entry.xIDPos) && ParseNumID())
      {
        m_Entry.Type = ID_FOUND; // we found an entry with a valid numeric id
        return true;
      }

      size_t plurPos;
      if (FindLineStart ("\nmsgid_plural ", plurPos))
      {
        m_Entry.Type = MSGID_PLURAL_FOUND; // we found a pluralized entry
        return true;
      }

      m_Entry.Type = MSGID_FOUND; // we found a normal entry, with no numeric id
      return true;
    }
  }
  while (m_nextEntryPos != m_POfilelength-1);
  // we reached the end of buffer AND we have not found a valid entry

  return false;
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

  if (m_Entry.Type != ID_FOUND)
  {
    GetString(m_Entry.msgID);
    if (FindLineStart ("\nmsgctxt ", m_Entry.msgCtxt.Pos))
      GetString(m_Entry.msgCtxt);
    else
      m_Entry.msgCtxt.Str.clear();
  }

  if (m_Entry.Type != MSGID_PLURAL_FOUND)
  {
    if (FindLineStart ("\nmsgstr ", m_Entry.msgStr.Pos))
    {
      GetString(m_Entry.msgStr);
      GetString(m_Entry.msgID);
    }
    else
    {
      CLog::Log(LOGERROR, "POParser: missing msgstr line in entry. Failed entry: %s",
                m_Entry.Content.c_str());
      m_Entry.msgStr.Str.clear();
    }
    return;
  }

  // We found a plural form entry. We read it into a vector of CStrEntry types
  m_Entry.msgStrPlural.clear();
  std::string strPattern = "\nmsgstr[0] ";
  CStrEntry strEntry;

  for (int n=0; n<7 ; n++)
  {
    strPattern[8] = static_cast<char>(n+'0');
    if (FindLineStart (strPattern, strEntry.Pos))
    {
      GetString(strEntry);
      if (strEntry.Str.empty())
        break;
      m_Entry.msgStrPlural.push_back(strEntry);
    }
    else
      break;
  }

  if (m_Entry.msgStrPlural.size() == 0)
  {
    CLog::Log(LOGERROR, "POParser: msgstr[] plural lines have zero valid strings. "
                        "Failed entry: %s", m_Entry.Content.c_str());
    m_Entry.msgStrPlural.resize(1); // Put 1 element with an empty string into the vector
  }

  return;
}

const std::string& CPODocument::GetPlurMsgstr(size_t plural) const
{
  if (m_Entry.msgStrPlural.size() < plural+1)
  {
    CLog::Log(LOGERROR, "POParser: msgstr[%i] plural field requested, but not found in PO file. "
                        "Failed entry: %s", static_cast<int>(plural), m_Entry.Content.c_str());
    plural = m_Entry.msgStrPlural.size()-1;
  }
  return m_Entry.msgStrPlural[plural].Str;
}

std::string CPODocument::UnescapeString(const std::string &strInput)
{
  std::string strOutput;
  if (strInput.empty())
    return strOutput;

  char oescchar;
  strOutput.reserve(strInput.size());
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
        break;
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
    strOutput.push_back(oescchar);
  }
  return strOutput;
}

bool CPODocument::FindLineStart(const std::string &strToFind, size_t &FoundPos)
{

  FoundPos = m_Entry.Content.find(strToFind);

  if (FoundPos == std::string::npos || FoundPos + strToFind.size() + 2 > m_Entry.Content.size())
    return false; // if we don't find the string or if we don't have at least one char after it

  FoundPos += strToFind.size(); // to set the pos marker to the exact start of the real data
  return true;
}

bool CPODocument::ParseNumID()
{
  if (isdigit(m_Entry.Content.at(m_Entry.xIDPos))) // verify if the first char is digit
  {
    // we check for the numeric id for the fist 10 chars (uint32)
    m_Entry.xID = strtol(&m_Entry.Content[m_Entry.xIDPos], NULL, 10);
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

  while (startPos < m_Entry.Content.size())
  {
    nextLFPos = m_Entry.Content.find("\n", startPos);
    if (nextLFPos == std::string::npos)
      nextLFPos = m_Entry.Content.size();

    // check syntax, if it really is a valid quoted string line
    if (nextLFPos-startPos < 2 ||  m_Entry.Content[startPos] != '\"' ||
        m_Entry.Content[nextLFPos-1] != '\"')
      break;

    strEntry.Str.append(m_Entry.Content, startPos+1, nextLFPos-2-startPos);
    startPos = nextLFPos+1;
  }

  strEntry.Str = UnescapeString(strEntry.Str);
}

void CPODocument::ConvertLineEnds(const std::string &filename)
{
  size_t foundPos = m_strBuffer.find_first_of("\r");
  if (foundPos == std::string::npos)
    return; // We have only Linux style line endings in the file, nothing to do

  if (foundPos+1 >= m_strBuffer.size() || m_strBuffer[foundPos+1] != '\n')
    CLog::Log(LOGDEBUG, "POParser: PO file has Mac Style Line Endings. "
              "Converted in memory to Linux LF for file: %s", filename.c_str());
  else
    CLog::Log(LOGDEBUG, "POParser: PO file has Win Style Line Endings. "
              "Converted in memory to Linux LF for file: %s", filename.c_str());

  std::string strTemp;
  strTemp.reserve(m_strBuffer.size());
  for (std::string::const_iterator it = m_strBuffer.begin(); it < m_strBuffer.end(); it++)
  {
    if (*it == '\r')
    {
      if (it+1 == m_strBuffer.end() || *(it+1) != '\n')
        strTemp.push_back('\n'); // convert Mac style line ending and continue
      continue; // we have Win style line ending so we exclude this CR now
    }
    strTemp.push_back(*it);
  }
  m_strBuffer.swap(strTemp);
  m_POfilelength = m_strBuffer.size();
}

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

#include "utils/POUtils.h"
#include "filesystem/File.h"
#include "utils/log.h"

CPODocument::CPODocument()
{
  m_id = 0;
  m_pBuffer = NULL;
  m_pCursor = NULL;
  m_POfilelength = 0;
  m_currentline = "";
  m_pLastCursor = NULL;
}

CPODocument::~CPODocument()
{
  if (m_pBuffer)
  {
    delete [] m_pBuffer;
    m_pBuffer = NULL;
  }
  m_pCursor = NULL;
  m_pLastCursor = NULL;
}

bool CPODocument::LoadFile(const CStdString &pofilename)
{
  XFILE::CFile file;
  if (!file.Open(pofilename)) return false;
  m_POfilelength = file.GetLength();
  if (m_POfilelength < 1)
  {
    file.Close();
    CLog::Log(LOGERROR, "POParser: wrong filesize read from file: %s", pofilename.c_str());
    return false;
  }
  m_pBuffer = new char[static_cast<size_t>(m_POfilelength+1)];
  m_pCursor = m_pBuffer;
  file.Read(m_pBuffer, m_POfilelength);
  file.Close();
  if (ParseHeader()) return true;
  CLog::Log(LOGERROR, "POParser: unable to read PO file header from file: %s", pofilename.c_str());
  return false;
}

bool CPODocument::GetNextEntry()
{
  if (m_pCursor == m_pLastCursor) m_pCursor++; // if nothing was parsed from last call
  while ((m_pCursor - m_pBuffer + 8) < m_POfilelength)
  {
    if (*(m_pCursor-1) == '\n')
    {
      if (m_pCursor[0] == '#' && m_pCursor[1] == ':' && m_pCursor[2] == ' '
        && m_pCursor[3] == 'i' && m_pCursor[4] == 'd' && m_pCursor[5] == ':')
      {
        m_entrytype = ID_FOUND;
        m_pLastCursor = m_pCursor;
        return true;
      }
      if (m_pCursor[0] == 'm' && m_pCursor[1] == 's' && m_pCursor[2] == 'g')
      {
        if (m_pCursor[3] == 'i' && m_pCursor[4] == 'd')
        {
          m_entrytype = MSGID_FOUND;
          m_pLastCursor = m_pCursor;
          return true;
        }
        if (m_pCursor[3] == 'c' && m_pCursor[4] == 't' && m_pCursor[5] == 'x' && m_pCursor[6] == 't')
        {
          m_entrytype = MSGCTXT_FOUND;
          m_pLastCursor = m_pCursor;
          return true;
        }
      }
    }
    m_pCursor++;
  }
  return false; // if we have the end of buffer
}

int CPODocument::GetEntryID()
{
  CStdString strID;
  strID.append( &m_pCursor[6], NextNonDigit( &m_pCursor[6], 11));
  m_id = atoi (strID.c_str());
  return m_id;
}

void CPODocument::ParseEntry()
{
  CStdString* pStr = NULL;
  m_msgctx = "";
  for (int i=0; i < 10; i++) m_msgid[i] = m_msgstr[i] = "";
  while (ReadLine())
  {
    if (IsEmptyLine()) return; // an empty line closes the the reading of the entry

    if (pStr && ReadStringLine(pStr,0)) continue; // we are reading a continous multilne string
    else pStr = NULL; // end of reading the multiline string

    if (HasPrefix(m_currentline, "msgctxt") && m_currentline.size() > 9)
    {
      pStr = &m_msgctx;
      if (!ReadStringLine(pStr,8))
      {
        CLog::Log(LOGERROR, "POParser: wrong msgctxt format for id: %i", m_id);
        pStr = NULL;
      }
    }
    else if (HasPrefix(m_currentline, "msgid") && m_currentline.size() > 7)
    {
      pStr = &m_msgid[0];
      if (!ReadStringLine(pStr,6))
      {
        CLog::Log(LOGERROR, "POParser: wrong msgid format for id: %i", m_id);
        pStr = NULL;
      }
    }
    else if (HasPrefix(m_currentline, "msgstr") && m_currentline.size() > 8)
    {
      pStr = &m_msgstr[0];
      if (!ReadStringLine(pStr,7))
      {
        CLog::Log(LOGERROR, "POParser: wrong msgstr format for id: %i", m_id);
        pStr = NULL;
      }
    }
    else pStr=NULL; // any non recognized line breaks the reading of a multiline string
  }
  return; // we have the end of buffer
}

bool CPODocument::ParseHeader()
{
  do
  {
    if (!ReadLine()) return false;
  }
  while (!HasPrefix(m_currentline, "msgid"));

  do
  {
    if (IsEmptyLine()) return true; // an empty line closes the reading of the header
  }
  while (ReadLine());
  return false; // not found the end of the header until reaching EOF
}

CStdString CPODocument::UnescapeString(CStdString &strInput)
{
  char oescchar;
  CStdString strOutput = "";
  for (CStdString::iterator it = strInput.begin(); it < strInput.end(); it++)
  {
    if (*it != '\\') strOutput += *it;
    else
    {
      if (it+1 == strInput.end())
      {
        CLog::Log(LOGERROR, "POParser: warning, unhandled escape character at line-end in entry id: %i", m_id);
        continue;
      }
      it++;
      switch (*it)
      {
        case 'a':  oescchar = '\a'; break;
        case 'b':  oescchar = '\b'; break;
        case 'v':  oescchar = '\v'; break;
        case 'n':  oescchar = '\n'; break;
        case 't':  oescchar = '\t'; break;
        case 'r':  oescchar = '\r'; break;
        case '"':  oescchar = '"' ; break;
        case '\\': oescchar = '\\'; break;
        default: 
        {
          CLog::Log(LOGERROR, "POParser: warning, unhandled escape character in entry id: %i", m_id);
          continue;
        }
      }
      strOutput += oescchar;
    }
  }
  return strOutput;
}

bool CPODocument::ReadStringLine(CStdString* pStrToAppend, int skip)
{
  int linesize = m_currentline.size(); 
  if (m_currentline[linesize-1] != '\"' || m_currentline[skip] != '\"') return false;
  pStrToAppend->append(m_currentline, skip + 1, linesize - skip - 2);
  return true;
}

bool CPODocument::HasPrefix(const CStdString &strLine, const CStdString &strPrefix)
{
  if (strLine.length() < strPrefix.length())
    return false;
  else
    return strLine.compare(0, strPrefix.length(), strPrefix) == 0;
}

bool CPODocument::ReadLine()
{
  long nbuff, offset;
  m_currentline = "";
  nbuff = m_pBuffer + m_POfilelength - m_pCursor;
  if (nbuff < 1) return false; // if we are at the end of buffer

  offset =  NextChar(m_pCursor, '\n', nbuff);
  long offset_end = offset;
  long offset_start = 0;

  while (IsWhitespace(m_pCursor[offset_start])) offset_start++; // check first non-whitespace char
  while (IsWhitespace(m_pCursor[offset_end])) offset_end--; // check last non whitespace char
  m_currentline.append(m_pCursor+offset_start, offset_end - offset_start);
  m_pCursor += offset +1;
  return true;
}

long CPODocument::NextChar(char* pString, char char2find, long n)
{
  int i=0;
  while (pString[i] != char2find && i < n) i++;
  return i;
}

long CPODocument::NextNonDigit(char* pString, long n)
{
  int i=0;
  while (CharIsDigit(pString[i]) && (i < n)) i++;
  return i;
}

bool CPODocument::CharIsDigit(char char2check)
{
  return (char2check >= '0' && char2check <= '9');
}

bool CPODocument::IsWhitespace(char char2check)
{
  return (char2check == ' ' || char2check == '\t'
    || char2check == '\v' || char2check == '\f');
}

bool CPODocument::IsEmptyLine()
{
  if (m_currentline.empty()) return true;
  else if (m_currentline == "\n") return true;
  else if (m_currentline[0] == '#')
  { // handle "# comment" style comments as empty lines
    if (m_currentline.size() == 1 || (m_currentline.size() >= 2 && isspace(m_currentline[1])))
      return true;
    else
      return false;
  }
  else
  { // if the line only contains whitespaces
    for(CStdString::iterator i = m_currentline.begin(); i != m_currentline.end(); ++i)
    {
      if (!IsWhitespace(*i))
        return false;
    }
  }
  return true;
}

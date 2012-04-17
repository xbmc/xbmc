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

#include <string>
#include <vector>
#include <stdint.h>

enum
{
  ID_FOUND,
  MSGID_FOUND,
  MSGID_PLURAL_FOUND
};

enum Boolean
{
  ISSOURCELANG=true
};

// Struct to hold current position and text of the string field in the main PO entry.
struct CStrEntry
{
    size_t Pos;
    std::string Str;
};

// Struct to collect all important data of the current processed entry.
struct CPOEntry
{
    int Type;
    uint32_t xID;
    size_t xIDPos;
    std::string Content;
    CStrEntry msgCtxt;
    CStrEntry msgID;
    CStrEntry msgStr;
    std::vector<CStrEntry> msgStrPlural;
};

class CPODocument
{
  public:
    CPODocument();
    ~CPODocument();

    /*! \brief Tries to load a PO file into a temporary memory buffer.
     * It also tries to parse the header of the PO file.
     \param pofilename filename of the PO file to load.
     \return true if the load was successful, unless return false
     */
    bool LoadFile(const std::string &pofilename);

    /*! \brief Fast jumps to the next entry in PO buffer.
     * Finds next entry started with "#: id:" or msgctx or msgid.
     * to be as fast as possible this does not even get the id number
     * just the type of the entry found. GetEntryID() has to be called
     * for getting the id. After that ParseEntry() needs a call for
     * actually getting the msg strings. The reason for this is to
     * have calls and checks as fast as possible generally and specially
     * for parsing weather tokens and to parse only the needed strings from
     * the fallback language (missing from the gui language translation)
     \return true if there was an entry found, false if reached the end of buffer
     */
    bool GetNextEntry();

    /*! \brief Gets the type of entry found with GetNextEntry.
     \return the type of entry: ID_FOUND || MSGID_FOUND || MSGID_PLURAL_FOUND
     */
    int GetEntryType() {return m_Entry.Type;}

    /*! \brief Parses the numeric ID from current entry.
     * This function can only be called right after GetNextEntry()
     * to make sure that we have a valid entry detected.
     \return parsed ID number
     */
    uint32_t GetEntryID() {return m_Entry.xID;}

    /*! \brief Parses current entry.
     * Reads msgid, msgstr, msgstr[x], msgctxt strings.
     * Note that this function also back-converts the c++ style escape sequences.
     * The function only parses the needed strings, considering if it is a source language file.
     \param bisSourceLang if we parse a source English file.
     * 
     */
    void ParseEntry(bool bisSourceLang);

    /*! \brief Gets the msgctxt string previously parsed by ParseEntry().
     \return string* containing the msgctxt string, unescaped and linked together.
     */
    std::string* GetMsgctxt() {return &m_Entry.msgCtxt.Str;}

    /*! \brief Gets the msgid string previously parsed by ParseEntry().
     \return string* containing the msgid string, unescaped and linked together.
     */
    std::string* GetMsgid() {return &m_Entry.msgID.Str;}

    /*! \brief Gets the msgstr string previously parsed by ParseEntry().
     \return string* containing the msgstr string, unescaped and linked together.
     */
    std::string* GetMsgstr() {return &m_Entry.msgStr.Str;}

    /*! \brief Gets the msgstr[x] string previously parsed by ParseEntry().
     \param plural the number of plural-form expected to get (0-6).
     \return string* containing the msgstr string, unescaped and linked together.
     */
    std::string* GetPlurMsgstr(size_t plural);

  protected:

    /*! \brief Converts c++ style char escape sequences back to char.
     * Supports: \a \v \n \t \r \" \0 \f \? \' \\
     \param strInput string contains the string to be unescaped.
     \return unescaped string.
     */
    std::string UnescapeString(std::string &strInput);

    /*! \brief Finds the position of line, starting with a given string in current entry.
     * This function can only be called after GetNextEntry()
     \param strToFind a string what we look for, at beggining of the lines.
     \param FoundPos will get the position where we found the line starting with the string.
     \return false if no line like that can be found in the entry (m_Entry)
     */
    bool FindLineStart(std::string strToFind, size_t &FoundPos);

    /*! \brief Reads, and links together the quoted strings found with ParseEntry().
     * This function can only be called after GetNextEntry() called.
     \param strEntry.Str a string where we get the appended string lines.
     \param strEntry.Pos the position in m_Entry.Content to start reading the string.
     */
    void GetString(CStrEntry &strEntry);

    /*! \brief Parses the numeric id and checks if it is valid.
     * This function can only be called after GetNextEntry()
     * It checks m_Entry.Content at position m_Entry.xIDPos for the numeric id.
     * The converted ID number goes into m_Entry.xID for public read out.
     \return false, if parse and convert of the id number was unsuccesful.
     */
    bool ParseNumID();

    // Temporary string buffer to read file in.
    std::string m_strBuffer;
    // Size of the string buffer.
    size_t m_POfilelength;

    // Current cursor position in m_strBuffer.
    size_t m_CursorPos;
    // The next PO entry position in m_strBuffer.
    size_t m_nextEntryPos;

    // Variable to hold all data of currently proceessed entry.
    CPOEntry m_Entry;
};

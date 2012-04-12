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

enum
{
  ID_FOUND,
  MSGID_FOUND,
  MSGCTXT_FOUND
};

class CPODocument
{
  public:
    CPODocument();
    ~CPODocument();

    /*! \brief Tries to load a PO file into a temporary buffer.
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
     * actually getting the msgstr strings. The reason for this is to
     * have calls and checks as fast as possible generally and specially
     * for parsing weather tokens.
     \return true if there was an entry found, false if reached the end of buffer
     */
    bool GetNextEntry();

    /*! \brief Gets the type of entry found with GetNextEntry.
     \return the type of entry: ID_FOUND || MSGID_FOUND || MSGCTXT_FOUND
     */
    int GetEntryType() {return m_entrytype;}

    /*! \brief Parses the numeric ID from the buffer at cursor position.
     * This function can only be called right after GetNextEntry()
     * to make sure that m_pCursor stands right at the beginning of and ID entry
     \return parsed ID number
     */
    int GetEntryID();

    /*! \brief Parses next entry.
     * Reads msgid, msgstr, msgctxt strings.
     * Note that this function does not yet back-convert escaped characters.
     * The conversion is done with the GetMsgid, GetMsgstr, GetMsgctxt functions.
     */
    void ParseEntry();

    /*! \brief Gets the msgctxt string previously parsed by ParseEntry().
     * This function also converts c++ style character escapes back to chars.
     \return string containing the msgctxt string, unescaped and linked together.
     */
    std::string GetMsgctxt() {return UnescapeString(m_msgctx);}

    /*! \brief Gets the msgid string previously parsed by ParseEntry().
     * This function also converts c++ style character escapes back to chars.
     \param plural the number of plural-form expected to get (0-9).
     \return string containing the msgid string, unescaped and linked together.
     */
    std::string GetMsgid(const unsigned plural) {return UnescapeString(m_msgid[plural]);}

    /*! \brief Gets the msgstr string previously parsed by ParseEntry().
     * This function also converts c++ style character escapes back to chars.
     \param plural the number of plural-form expected to get (0-9).
     \return string containing the msgstr string, unescaped and linked together.
     */
    std::string GetMsgstr(const unsigned plural) {return UnescapeString(m_msgstr[plural]);}

  protected:

    /*! \brief Parses the header and sets the cursor to the end of the header.
     * It does not actually get any useful information at the moment,
     * since we don't need them in xbmc. Just sets cursor at right position.
     \return true if we found the header, false when no header was found.
     */
    bool ParseHeader();

    /*! \brief Converts c++ style char escapes back to char.
     \param strInput string contains the string to be unescaped.
     \return unescaped string.
     */
    std::string UnescapeString(std::string &strInput);

    /*! \brief Reads msg strings from current line, from position "skip".
     * Note that the msg strings should always start and end with apostrophe.
     * This appends the read string to an existing one, pointed by a pointer.
     \param skip The position we should start reading the string.
     \param pStrToAppend Pointer to a string where we should add the read string.
     \return false if the string is not surrounded by apostrophe chars.
     */
    bool ReadStringLine(std::string * pStrToAppend, int skip);

    /*! \brief Checks if strLine starts with strPrefix.
     \param strLine The string to check.
     \param strPrefix The string containing the prefix to look for.
     \return true if the string starts with the prefix.
     */
    bool HasPrefix(const std::string &strLine, const std::string &strPrefix);

    /*! \brief Reads the next line from the buffer.
     * It also removes trailing and leading whitespace from the read line.
     * The successfully read line gets stored in m_currentline.
     \return false if we try to read data over the end of the buffer.
     */
    bool ReadLine();

    /*! \brief Look for a given character in string pointed by pString.
     \param char2find The character we are looking for.
     \param pString Pointer to the string where we look for the char.
     \param n Number of characters we go before stop searching.
     \return position of the found character.
     */
    long NextChar(char* pString, char char2find, long n);

    /*! \brief Look for the first non-number character from pString.
     \param pString Pointer to the string where we look for nondigit.
     \param n Number of characters we go before stop searching.
     \return position of the found character.
     */
    long NextNonDigit(char* pString, long n);

    /*! \brief Check if character is whitespace.
     * We actually look for: ' ', '\t', '\v", '\f' NOT '\n'
     \param char2check The input char to evaluate.
     \return true if char was whitespace.
     */
    bool IsWhitespace(char char2check);

    /*! \brief Checks if the string in m_currentline is an empty line or a comment.
     * For this function to work, we need ReadLine() to fill in m_currentline.
     * The function considers comments in "# Comment" style as an empty lines.
     * Also if the line only contains whitespace, it is handled as an empty line.
     \return true, if m_currentline contained an empty line or a comment.
     */
    bool IsEmptyLine();

    char* m_pBuffer;
    char* m_pCursor;
    char* m_pLastCursor;
    int m_id;
    int m_entrytype;
    std::string m_msgid[10];
    std::string m_msgstr[10];
    std::string m_msgctx;
    size_t m_POfilelength;
    std::string m_currentline;
};

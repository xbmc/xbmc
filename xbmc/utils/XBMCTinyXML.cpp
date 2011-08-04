/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "XBMCTinyXML.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/File.h"
#include "utils/CharsetConverter.h"

CXBMCTinyXML::CXBMCTinyXML()
{
  convertToUtf8 = false;
}

CXBMCTinyXML::CXBMCTinyXML(const char *documentName)
{
  LoadFile(documentName);
}

bool CXBMCTinyXML::LoadFile(const char *_filename, TiXmlEncoding encoding)
{
  // There was a really terrifying little bug here. The code:
  //    value = filename
  // in the STL case, cause the assignment method of the std::string to
  // be called. What is strange, is that the std::string had the same
  // address as it's c_str() method, and so bad things happen. Looks
  // like a bug in the Microsoft STL implementation.
  // Add an extra string to avoid the crash.
  TIXML_STRING filename( _filename );
  value = filename;

  XFILE::CFile file;
  if (!file.Open(value))
  {
    SetError( TIXML_ERROR_OPENING_FILE, 0, 0, TIXML_ENCODING_UNKNOWN );
    return false;
  }

  // Delete the existing data:
  Clear();
  location.Clear();

  // Get the file size, so we can pre-allocate the string. HUGE speed impact.
  long length = -1;
  int64_t filelen = file.GetLength();
  if (filelen > 0)
    length = (long)filelen;

  // We might be streaming it, correct length will be fixed by reading
  if( length < 0 )
    length = 1024;

  // Subtle bug here. TinyXml did use fgets. But from the XML spec:
  // 2.11 End-of-Line Handling
  // <snip>
  // <quote>
  // ...the XML processor MUST behave as if it normalized all line breaks in external
  // parsed entities (including the document entity) on input, before parsing, by translating
  // both the two-character sequence #xD #xA and any #xD that is not followed by #xA to
  // a single #xA character.
  // </quote>
  //
  // It is not clear fgets does that, and certainly isn't clear it works cross platform.
  // Generally, you expect fgets to translate from the convention of the OS to the c/unix
  // convention, and not work generally.

  /*
  while( fgets( buf, sizeof(buf), file ) )
  {
    data += buf;
  }
  */

  char*  buf = (char*)malloc(length+1);
  long   pos = 0;
  long   len;
  while( (len = file.Read(buf+pos, length-pos)) > 0 ) {
    pos += len;
    assert(pos <= length);
    if(pos == length) {
      length *= 2;
      buf = (char*)realloc(buf, length);
    }
  }
  length = pos;

  file.Close();

  // Strange case, but good to handle up front.
  if ( length == 0 )
  {
    SetError( TIXML_ERROR_DOCUMENT_EMPTY, 0, 0, TIXML_ENCODING_UNKNOWN );
    return false;
  }

  // If we have a file, assume it is all one big XML file, and read it in.
  // The document parser may decide the document ends sooner than the entire file, however.
  TIXML_STRING data;
  data.reserve( length );

  const char* lastPos = buf;
  const char* p = buf;

  buf[length] = 0;
  while( *p ) {
    assert( p < (buf+length) );
    if ( *p == 0xa ) {
      // Newline character. No special rules for this. Append all the characters
      // since the last string, and include the newline.
      data.append( lastPos, (p-lastPos+1) );  // append, include the newline
      ++p;                  // move past the newline
      lastPos = p;              // and point to the new buffer (may be 0)
      assert( p <= (buf+length) );
    }
    else if ( *p == 0xd ) {
      // Carriage return. Append what we have so far, then
      // handle moving forward in the buffer.
      if ( (p-lastPos) > 0 ) {
        data.append( lastPos, p-lastPos );  // do not add the CR
      }
      data += (char)0xa;            // a proper newline

      if ( *(p+1) == 0xa ) {
        // Carriage return - new line sequence
        p += 2;
        lastPos = p;
        assert( p <= (buf+length) );
      }
      else {
        // it was followed by something else...that is presumably characters again.
        ++p;
        lastPos = p;
        assert( p <= (buf+length) );
      }
    }
    else {
      ++p;
    }
  }
  // Handle any left over characters.
  if ( p-lastPos ) {
    data.append( lastPos, p-lastPos );
  }
  free(buf);
  buf = 0;

  if (convertToUtf8)
  {
    CStdString temp;
    g_charsetConverter.unknownToUTF8(data, temp);
    data = temp;
  }

  Parse( data.c_str(), 0, encoding );

  if (  Error() )
    return false;
  else
    return true;
}

bool CXBMCTinyXML::SaveFile(const char *filename) const
{
  XFILE::CFile file;
  if (file.OpenForWrite(filename, true))
  {
    TiXmlPrinter printer;
    Accept(&printer);
    file.Write(printer.CStr(), printer.Size());
    return true;
  }
  return false;
}

void CXBMCTinyXML::setConvertToUtf8(bool value)
{
  convertToUtf8 = value;
}

bool CXBMCTinyXML::getConvertToUtf8()
{
  return convertToUtf8;
}

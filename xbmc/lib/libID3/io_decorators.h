// -*- C++ -*-
// $Id$

// id3lib: a software library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug
// Copyright 2002 Thijmen Klok (thijmen@id3lib.org)

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#ifndef _ID3LIB_READER_DECORATORS_H_
#define _ID3LIB_READER_DECORATORS_H_

#if defined(__BORLANDC__)
// due to a bug in borland it sometimes still wants mfc compatibility even when you disable it
#  if defined(_MSC_VER)
#    undef _MSC_VER
#  endif
#  if defined(__MFC_COMPAT__)
#    undef __MFC_COMPAT__
#  endif
#endif

#include "readers.h"
#include "io_helpers.h"
#include "id3/utils.h" // has <config.h> "id3/id3lib_streams.h" "id3/globals.h" "id3/id3lib_strings.h"

namespace dami
{
  namespace io
  {
    /**
     * Set a window on the buffer.  Characters can only be read within this
     * window.
     */
    class ID3_CPP_EXPORT WindowedReader : public ID3_Reader
    {
      typedef ID3_Reader SUPER;

      ID3_Reader& _reader;
      pos_type _beg, _end;

      bool inWindow(pos_type cur)
      { return this->getBeg() <= cur && cur < this->getEnd(); }

     public:
      explicit WindowedReader(ID3_Reader& reader)
        : _reader(reader), _beg(reader.getBeg()), _end(reader.getEnd()) { ; }

      WindowedReader(ID3_Reader& reader, size_type size)
        : _reader(reader), _beg(reader.getBeg()), _end(reader.getEnd())
      { this->setWindow(this->getCur(), size); }

      WindowedReader(ID3_Reader& reader, pos_type beg, size_type size)
        : _reader(reader), _beg(reader.getBeg()), _end(reader.getEnd())
      { this->setWindow(beg, size); }

      void setWindow(pos_type beg, size_type size);

      pos_type setBeg(pos_type);
      pos_type setCur(pos_type cur)
      {
        return _reader.setCur(mid(this->getBeg(), cur, this->getEnd()));
      }
      pos_type setEnd(pos_type);

      pos_type getCur() { return _reader.getCur(); }
      pos_type getBeg() { return _beg; }
      pos_type getEnd() { return _end; }

      bool inWindow() { return this->inWindow(this->getCur()); }

      int_type readChar();
      int_type peekChar();

      size_type readChars(char_type buf[], size_type len);
      size_type readChars(char buf[], size_type len)
      {
        return this->readChars((char_type*) buf, len);
      }

      void close() { ; }
    };

    class ID3_CPP_EXPORT CharReader : public ID3_Reader
    {
      typedef ID3_Reader SUPER;

     protected:
      ID3_Reader& _reader;

    public:

      CharReader(ID3_Reader& reader) : _reader(reader) { }
      virtual ~CharReader() { ; }

      /**
       * Read \c len characters into the array \c buf.  Since the stream needs
       * might have been unsynced, this function copies the characters one at a
       * time.
       */
      size_type readChars(char_type buf[], size_type len);
      size_type readChars(char buf[], size_type len)
      {
        return this->readChars((char_type*) buf, len);
      }

      void close() { ; }
      int_type peekChar() { return _reader.peekChar(); }

      pos_type getBeg() { return _reader.getBeg(); }
      pos_type getCur() { return _reader.getCur(); }
      pos_type getEnd() { return _reader.getEnd(); }

      pos_type setCur(pos_type cur) { return _reader.setCur(cur); }
    };


    class ID3_CPP_EXPORT LineFeedReader : public CharReader
    {
      typedef CharReader SUPER;

     public:
      LineFeedReader(ID3_Reader& reader) : SUPER(reader) { ; }
      int_type readChar();
    };

    class ID3_CPP_EXPORT UnsyncedReader : public CharReader
    {
      typedef CharReader SUPER;

     public:
      UnsyncedReader(ID3_Reader& reader) : SUPER(reader) { }
      int_type readChar();
    };

    class ID3_CPP_EXPORT CompressedReader : public ID3_MemoryReader
    {
      char_type* _uncompressed;
     public:
      CompressedReader(ID3_Reader& reader, size_type newSize);
      virtual ~CompressedReader();
    };

    class ID3_CPP_EXPORT UnsyncedWriter : public ID3_Writer
    {
      typedef ID3_Writer SUPER;

      ID3_Writer& _writer;
      int_type _last;
      size_type _numSyncs;

     public:
      UnsyncedWriter(ID3_Writer& writer)
        : _writer(writer), _last('\0'), _numSyncs(0)
      { ; }

      size_type getNumSyncs() const { return _numSyncs; }
      int_type writeChar(char_type ch);
      void flush();

      /**
       * Write \c len characters into the array \c buf.  Since the stream needs
       * might have been unsynced, this function copies the characters one at a
       * time.
       */
      size_type writeChars(const char_type[], size_type len);
      size_type writeChars(const char buf[], size_type len)
      {
        return this->writeChars(reinterpret_cast<const char_type *>(buf), len);
      }

      void close() { ; }

      pos_type getBeg() { return _writer.getBeg(); }
      pos_type getCur() { return _writer.getCur(); }
      pos_type getEnd() { return _writer.getEnd(); }
    };

    class CompressedWriter : public ID3_Writer
    {
      typedef ID3_Writer SUPER;

      ID3_Writer& _writer;
      BString _data;
      size_type _origSize;
     public:

      explicit CompressedWriter(ID3_Writer& writer)
        : _writer(writer), _data(), _origSize(0)
      { ; }
      virtual ~CompressedWriter() { this->flush(); }

      size_type getOrigSize() const { return _origSize; }

      void flush();

      size_type writeChars(const char_type buf[], size_type len);
      size_type writeChars(const char buf[], size_type len)
      {
        return this->writeChars(reinterpret_cast<const char_type*>(buf), len);
      }

      pos_type getCur() { return _data.size(); }
      void close() { ; }
    };
  };
};

#endif /* _ID3LIB_READER_DECORATORS_H_ */


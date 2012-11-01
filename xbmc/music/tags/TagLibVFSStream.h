#pragma once
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
#include "filesystem/File.h"
#include "utils/StdString.h"
#include <taglib/tiostream.h>

using namespace XFILE;
using namespace TagLib;

namespace MUSIC_INFO
{
  class TagLibVFSStream : public IOStream
  {
  public:
    /*!
     * Construct a File object and opens the \a file.  \a file should be a
     * be an XBMC Vfile.
     */
    TagLibVFSStream(const std::string& strFileName, bool readOnly);

    /*!
     * Destroys this ByteVectorStream instance.
     */
    virtual ~TagLibVFSStream();
    
    /*!
     * Returns the file name in the local file system encoding.
     */
    FileName name() const;

    /*!
     * Reads a block of size \a length at the current get pointer.
     */
    ByteVector readBlock(TagLib::ulong length);

    /*!
     * Attempts to write the block \a data at the current get pointer.  If the
     * file is currently only opened read only -- i.e. readOnly() returns true --
     * this attempts to reopen the file in read/write mode.
     *
     * \note This should be used instead of using the streaming output operator
     * for a ByteVector.  And even this function is significantly slower than
     * doing output with a char[].
     */
    void writeBlock(const ByteVector &data);

    /*!
     * Insert \a data at position \a start in the file overwriting \a replace
     * bytes of the original content.
     *
     * \note This method is slow since it requires rewriting all of the file
     * after the insertion point.
     */
    void insert(const ByteVector &data, TagLib::ulong start = 0, TagLib::ulong replace = 0);

    /*!
     * Removes a block of the file starting a \a start and continuing for
     * \a length bytes.
     *
     * \note This method is slow since it involves rewriting all of the file
     * after the removed portion.
     */
    void removeBlock(TagLib::ulong start = 0, TagLib::ulong length = 0);

    /*!
     * Returns true if the file is read only (or if the file can not be opened).
     */
    bool readOnly() const;

    /*!
     * Since the file can currently only be opened as an argument to the
     * constructor (sort-of by design), this returns if that open succeeded.
     */
    bool isOpen() const;

    /*!
     * Move the I/O pointer to \a offset in the file from position \a p.  This
     * defaults to seeking from the beginning of the file.
     *
     * \see Position
     */
    void seek(long offset, Position p = Beginning);

    /*!
     * Reset the end-of-file and error flags on the file.
     */
    void clear();

    /*!
     * Returns the current offset within the file.
     */
    long tell() const;

    /*!
     * Returns the length of the file.
     */
    long length();

    /*!
     * Truncates the file to a \a length.
     */
    void truncate(long length);

  protected:
    /*!
     * Returns the buffer size that is used for internal buffering.
     */
    static TagLib::uint bufferSize() { return 1024; };

  private:
    std::string m_strFileName;
    CFile       m_file;
    bool        m_bIsReadOnly;
    bool        m_bIsOpen;
    int         m_bufferSize;
  };
}


/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/File.h"

#include <taglib/tiostream.h>

namespace MUSIC_INFO
{
  class TagLibVFSStream : public TagLib::IOStream
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
    ~TagLibVFSStream() override;

    /*!
     * Returns the file name in the local file system encoding.
     */
    TagLib::FileName name() const override;

    /*!
     * Reads a block of size \a length at the current get pointer.
     */
    TagLib::ByteVector readBlock(TagLib::ulong length) override;

    /*!
     * Attempts to write the block \a data at the current get pointer.  If the
     * file is currently only opened read only -- i.e. readOnly() returns true --
     * this attempts to reopen the file in read/write mode.
     *
     * \note This should be used instead of using the streaming output operator
     * for a ByteVector.  And even this function is significantly slower than
     * doing output with a char[].
     */
    void writeBlock(const TagLib::ByteVector &data) override;

    /*!
     * Insert \a data at position \a start in the file overwriting \a replace
     * bytes of the original content.
     *
     * \note This method is slow since it requires rewriting all of the file
     * after the insertion point.
     */
    void insert(const TagLib::ByteVector &data, TagLib::ulong start = 0, TagLib::ulong replace = 0) override;

    /*!
     * Removes a block of the file starting a \a start and continuing for
     * \a length bytes.
     *
     * \note This method is slow since it involves rewriting all of the file
     * after the removed portion.
     */
    void removeBlock(TagLib::ulong start = 0, TagLib::ulong length = 0) override;

    /*!
     * Returns true if the file is read only (or if the file can not be opened).
     */
    bool readOnly() const override;

    /*!
     * Since the file can currently only be opened as an argument to the
     * constructor (sort-of by design), this returns if that open succeeded.
     */
    bool isOpen() const override;

    /*!
     * Move the I/O pointer to \a offset in the file from position \a p.  This
     * defaults to seeking from the beginning of the file.
     *
     * \see Position
     */
    void seek(long offset, TagLib::IOStream::Position p = Beginning) override;

    /*!
     * Reset the end-of-file and error flags on the file.
     */
    void clear() override;

    /*!
     * Returns the current offset within the file.
     */
    long tell() const override;

    /*!
     * Returns the length of the file.
     */
    long length() override;

    /*!
     * Truncates the file to a \a length.
     */
    void truncate(long length) override;

  protected:
    /*!
     * Returns the buffer size that is used for internal buffering.
     */
    static TagLib::uint bufferSize() { return 1024; }

  private:
    std::string   m_strFileName;
    XFILE::CFile  m_file;
    bool          m_bIsReadOnly;
    bool          m_bIsOpen;
  };
}


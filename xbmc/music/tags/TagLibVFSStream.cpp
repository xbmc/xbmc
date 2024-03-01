/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "TagLibVFSStream.h"

#include "filesystem/File.h"

#include <limits.h>

#include <taglib/taglib.h>
#include <taglib/tiostream.h>

using namespace XFILE;
using namespace TagLib;
using namespace MUSIC_INFO;

/*!
 * Construct a File object and opens the \a file.  \a file should be a
 * be an XBMC Vfile.
 */
TagLibVFSStream::TagLibVFSStream(const std::string& strFileName, bool readOnly)
{
  m_bIsOpen = true;
  if (readOnly)
  {
    if (!m_file.Open(strFileName))
      m_bIsOpen = false;
  }
  else
  {
    if (!m_file.OpenForWrite(strFileName))
      m_bIsOpen = false;
  }
  m_strFileName = strFileName;
  m_bIsReadOnly = readOnly || !m_bIsOpen;
}

/*!
 * Destroys this ByteVectorStream instance.
 */
TagLibVFSStream::~TagLibVFSStream()
{
  m_file.Close();
}

/*!
 * Returns the file name in the local file system encoding.
 */
FileName TagLibVFSStream::name() const
{
  return m_strFileName.c_str();
}

/*!
 * Reads a block of size \a length at the current get pointer.
 */
#if (TAGLIB_MAJOR_VERSION >= 2)
ByteVector TagLibVFSStream::readBlock(unsigned long length)
#else
ByteVector TagLibVFSStream::readBlock(TagLib::ulong length)
#endif
{
#if (TAGLIB_MAJOR_VERSION >= 2)
  ByteVector byteVector(static_cast<unsigned int>(length));
#else
  ByteVector byteVector(static_cast<TagLib::uint>(length));
#endif
  ssize_t read = m_file.Read(byteVector.data(), length);
  if (read > 0)
    byteVector.resize(read);
  else
    byteVector.clear();

  return byteVector;
}

/*!
 * Attempts to write the block \a data at the current get pointer.  If the
 * file is currently only opened read only -- i.e. readOnly() returns true --
 * this attempts to reopen the file in read/write mode.
 *
 * \note This should be used instead of using the streaming output operator
 * for a ByteVector.  And even this function is significantly slower than
 * doing output with a char[].
 */
void TagLibVFSStream::writeBlock(const ByteVector &data)
{
  m_file.Write(data.data(), data.size());
}

/*!
 * Insert \a data at position \a start in the file overwriting \a replace
 * bytes of the original content.
 *
 * \note This method is slow since it requires rewriting all of the file
 * after the insertion point.
 */
#if (TAGLIB_MAJOR_VERSION >= 2)
void TagLibVFSStream::insert(const ByteVector& data, TagLib::offset_t start, size_t replace)
#else
void TagLibVFSStream::insert(const ByteVector &data, TagLib::ulong start, TagLib::ulong replace)
#endif
{
  if (data.size() == replace)
  {
    seek(start);
    writeBlock(data);
    return;
  }
  else if (data.size() < replace)
  {
    seek(start);
    writeBlock(data);
    removeBlock(start + data.size(), replace - data.size());
  }

  // Woohoo!  Faster (about 20%) than id3lib at last.  I had to get hardcore
  // and avoid TagLib's high level API for rendering just copying parts of
  // the file that don't contain tag data.
  //
  // Now I'll explain the steps in this ugliness:

  // First, make sure that we're working with a buffer that is longer than
  // the *difference* in the tag sizes.  We want to avoid overwriting parts
  // that aren't yet in memory, so this is necessary.
#if (TAGLIB_MAJOR_VERSION >= 2)
  unsigned long bufferLength = bufferSize();
#else
  TagLib::ulong bufferLength = bufferSize();
#endif

  while (data.size() - replace > bufferLength)
    bufferLength += bufferSize();

  // Set where to start the reading and writing.
  long readPosition = start + replace;
  long writePosition = start;
  ByteVector buffer;
#if (TAGLIB_MAJOR_VERSION >= 2)
  ByteVector aboutToOverwrite(static_cast<unsigned int>(bufferLength));
#else
  ByteVector aboutToOverwrite(static_cast<TagLib::uint>(bufferLength));
#endif

  // This is basically a special case of the loop below.  Here we're just
  // doing the same steps as below, but since we aren't using the same buffer
  // size -- instead we're using the tag size -- this has to be handled as a
  // special case.  We're also using File::writeBlock() just for the tag.
  // That's a bit slower than using char *'s so, we're only doing it here.
  seek(readPosition);
  ssize_t bytesRead = m_file.Read(aboutToOverwrite.data(), bufferLength);
  if (bytesRead <= 0)
    return; // error
  readPosition += bufferLength;

  seek(writePosition);
  writeBlock(data);
  writePosition += data.size();

  buffer = aboutToOverwrite;
  buffer.resize(bytesRead);

  // Ok, here's the main loop.  We want to loop until the read fails, which
  // means that we hit the end of the file.
  while (!buffer.isEmpty())
  {
    // Seek to the current read position and read the data that we're about
    // to overwrite.  Appropriately increment the readPosition.
    seek(readPosition);
    bytesRead = m_file.Read(aboutToOverwrite.data(), bufferLength);
    if (bytesRead <= 0)
      return; // error
    aboutToOverwrite.resize(bytesRead);
    readPosition += bufferLength;

    // Check to see if we just read the last block.  We need to call clear()
    // if we did so that the last write succeeds.
#if (TAGLIB_MAJOR_VERSION >= 2)
    if (static_cast<unsigned long>(bytesRead) < bufferLength)
#else
    if (TagLib::ulong(bytesRead) < bufferLength)
#endif
      clear();

    // Seek to the write position and write our buffer.  Increment the
    // writePosition.
    seek(writePosition);
    if (m_file.Write(buffer.data(), buffer.size()) < static_cast<ssize_t>(buffer.size()))
      return; // error
    writePosition += buffer.size();

    buffer = aboutToOverwrite;
    bufferLength = bytesRead;
  }
}

/*!
 * Removes a block of the file starting a \a start and continuing for
 * \a length bytes.
 *
 * \note This method is slow since it involves rewriting all of the file
 * after the removed portion.
 */
#if (TAGLIB_MAJOR_VERSION >= 2)
void TagLibVFSStream::removeBlock(TagLib::offset_t start, size_t length)
#else
void TagLibVFSStream::removeBlock(TagLib::ulong start, TagLib::ulong length)
#endif
{
#if (TAGLIB_MAJOR_VERSION >= 2)
  unsigned long bufferLength = bufferSize();
#else
  TagLib::ulong bufferLength = bufferSize();
#endif

  long readPosition = start + length;
  long writePosition = start;

#if (TAGLIB_MAJOR_VERSION >= 2)
  ByteVector buffer(static_cast<unsigned int>(bufferLength));
#else
  ByteVector buffer(static_cast<TagLib::uint>(bufferLength));
#endif

#if (TAGLIB_MAJOR_VERSION >= 2)
  unsigned long bytesRead = 1;
#else
  TagLib::ulong bytesRead = 1;
#endif

  while(bytesRead != 0)
  {
    seek(readPosition);
    ssize_t read = m_file.Read(buffer.data(), bufferLength);
    if (read < 0)
      return;// explicit error

#if (TAGLIB_MAJOR_VERSION >= 2)
    bytesRead = static_cast<unsigned long>(read);
#else
    bytesRead = static_cast<TagLib::ulong>(read);
#endif
    readPosition += bytesRead;

    // Check to see if we just read the last block.  We need to call clear()
    // if we did so that the last write succeeds.
    if(bytesRead < bufferLength)
      clear();

    seek(writePosition);
    if (m_file.Write(buffer.data(), bytesRead) != static_cast<ssize_t>(bytesRead))
      return; // error
    writePosition += bytesRead;
  }
  truncate(writePosition);
}

/*!
 * Returns true if the file is read only (or if the file can not be opened).
 */
bool TagLibVFSStream::readOnly() const
{
  return m_bIsReadOnly;
}

/*!
 * Since the file can currently only be opened as an argument to the
 * constructor (sort-of by design), this returns if that open succeeded.
 */
bool TagLibVFSStream::isOpen() const
{
  return m_bIsOpen;
}

/*!
 * Move the I/O pointer to \a offset in the file from position \a p.  This
 * defaults to seeking from the beginning of the file.
 *
 * \see Position
 */
void TagLibVFSStream::seek(long offset, Position p)
{
  const long fileLen = length();
  if (m_bIsReadOnly && fileLen > 0)
  {
    long startPos;
    if (p == Beginning)
      startPos = 0;
    else if (p == Current)
      startPos = tell();
    else if (p == End)
      startPos = fileLen;
    else
      return; // wrong Position value

    // When parsing some broken files, taglib may try to seek above end of file.
    // If underlying VFS does not move I/O pointer in this case, taglib will parse
    // same part of file several times and ends with error. To prevent this
    // situation, force seek to last valid position so VFS move I/O pointer.
    if (startPos >= 0)
    {
      if (offset < 0 && startPos + offset < 0)
      {
        m_file.Seek(0, SEEK_SET);
        return;
      }
      if (offset > 0 && startPos + offset > fileLen)
      {
        m_file.Seek(fileLen, SEEK_SET);
        return;
      }
    }
  }

  switch(p)
  {
    case Beginning:
      m_file.Seek(offset, SEEK_SET);
      break;
    case Current:
      m_file.Seek(offset, SEEK_CUR);
      break;
    case End:
      m_file.Seek(offset, SEEK_END);
      break;
  }
}

/*!
 * Reset the end-of-file and error flags on the file.
 */
void TagLibVFSStream::clear()
{
}

/*!
 * Returns the current offset within the file.
 */
long TagLibVFSStream::tell() const
{
  int64_t pos = m_file.GetPosition();
  if(pos > LONG_MAX)
    return -1;
  else
    return (long)pos;
}

/*!
 * Returns the length of the file.
 */
long TagLibVFSStream::length()
{
  return (long)m_file.GetLength();
}

/*!
 * Truncates the file to a \a length.
 */
void TagLibVFSStream::truncate(long length)
{
  m_file.Truncate(length);
}

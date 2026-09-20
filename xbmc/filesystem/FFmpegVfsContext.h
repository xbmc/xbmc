/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

struct AVFormatContext;
struct AVIOContext;

namespace XFILE
{
class CFile;

/*!
 * \brief A file opened through the VFS for FFmpeg to demux: the CFile, the AVIOContext reading
 *        through it and the AVFormatContext reading through that, opened and closed as one.
 *
 * The three are one lifetime, because FFmpeg's IO callbacks hold a pointer to the CFile. Letting
 * one of these leave scope unwinds however far the open got, so no caller carries that itself.
 */
class CFFmpegVfsContext
{
public:
  // Both are out of line because m_file is a unique_ptr to a forward declared CFile: either one
  // defaulted here would need the complete type in every translation unit including this.
  CFFmpegVfsContext();
  ~CFFmpegVfsContext();

  CFFmpegVfsContext(const CFFmpegVfsContext&) = delete;
  CFFmpegVfsContext& operator=(const CFFmpegVfsContext&) = delete;

  /*!
   * \brief Open a file and probe it, leaving a context ready to be read for tags, chapters,
   *        attachments or stream properties.
   * \param path What the VFS opens and FFmpeg probes.
   * \return Whether the file was opened. Nothing is left open when it was not.
   *
   * Whatever was open is closed first, so one of these can be reopened on another file.
   */
  bool Open(const std::string& path);

  //! Closes what Open() left, if anything, and leaves this reusable.
  void Close();

  bool IsOpen() const { return m_fctx != nullptr; }

  /*!
   * \brief Whether the demuxer could read the streams' properties.
   * \return False where avformat_find_stream_info() failed, which leaves sample rate, channel
   *         count and stream duration unset rather than measured.
   *
   * Open() succeeds either way, because the tags, the chapters and the attachments come from the
   * header and are there regardless. Anything read off the streams has to ask this first, or it
   * reports what was never measured as if it had been.
   */
  bool HasStreamInfo() const { return m_haveStreamInfo; }

  /*!
   * \brief What the demuxer makes of the file's length.
   * \return Seconds, or 0 where nothing is open or the demuxer could not tell - which is a real
   *         answer for a container that declares none and holds too little to measure.
   */
  double Duration() const;

  //! The demuxer context, or nullptr when nothing is open. Owned here, only borrowed by callers.
  AVFormatContext* FormatContext() const { return m_fctx; }

private:
  //! The file the IO callbacks read: it has to outlive the contexts that point at it.
  std::unique_ptr<CFile> m_file;
  AVIOContext* m_ioctx{nullptr};
  AVFormatContext* m_fctx{nullptr};
  bool m_haveStreamInfo{false};
};
} // namespace XFILE

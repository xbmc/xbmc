/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Geometry.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct kodi_web_video_frame;

namespace KODI
{
namespace WEB
{

/*!
 * \brief The connection of one of the web client's video sinks
 */
class CWebVideoConnection
{
public:
  explicit CWebVideoConnection(int fd);
  ~CWebVideoConnection();

  int GetFd() const { return m_fd; }

  /*!
   * \brief Tell the sink Kodi no longer shows its frame; any thread
   */
  void Release(uint32_t id);

private:
  int m_fd;
};

/*!
 * \brief A frame a video sink handed over; Kodi scans it out from the sink's
 * memory, and the sink gets it back once the frame returns to its pool
 */
class CWebVideoFrame : public CVideoBufferDRMPRIME
{
public:
  explicit CWebVideoFrame(int id);
  ~CWebVideoFrame() override;

  void Set(std::shared_ptr<CWebVideoConnection> connection,
           const kodi_web_video_frame& frame,
           const std::vector<int>& fds);
  void Clear();

  uint32_t GetWidth() const override { return m_width; }
  uint32_t GetHeight() const override { return m_height; }
  AVDRMFrameDescriptor* GetDescriptor() const override;
  bool PrefersPlane() const override { return true; }

  uint32_t GetParN() const { return m_parN; }
  uint32_t GetParD() const { return m_parD; }
  uint32_t GetMatrix() const { return m_matrix; }
  uint32_t GetRange() const { return m_range; }
  uint32_t GetTransfer() const { return m_transfer; }

private:
  std::shared_ptr<CWebVideoConnection> m_connection;
  uint32_t m_frameId{0};
  uint32_t m_width{0};
  uint32_t m_height{0};
  uint32_t m_parN{1};
  uint32_t m_parD{1};
  uint32_t m_matrix{0};
  uint32_t m_range{0};
  uint32_t m_transfer{0};
  AVDRMFrameDescriptor m_descriptor{};
};

class CWebVideoFramePool : public IVideoBufferPool
{
public:
  ~CWebVideoFramePool() override;

  // implementation of IVideoBufferPool
  CVideoBuffer* Get() override;
  void Return(int id) override;

private:
  CCriticalSection m_critSection;
  std::vector<CWebVideoFrame*> m_all;
  std::deque<int> m_free;
};

/*!
 * \brief Receives the frames of the web client's video
 */
class IWebVideoObserver
{
public:
  virtual ~IWebVideoObserver() = default;

  /*!
   * \brief A frame is due; the observer takes over one reference to it
   */
  virtual void OnWebVideoFrame(CWebVideoFrame* frame) = 0;

  /*!
   * \brief The video shows nothing until its next frame
   */
  virtual void OnWebVideoClear() = 0;

  /*!
   * \brief Where on the view the video shows, in the view's pixels; an empty
   * rectangle leaves the placement to the renderer
   */
  virtual void OnWebVideoRect(const CRect& rect) = 0;
};

/*!
 * \brief Serves the socket the web client's video sinks connect to, so the
 * video of web pages is shown on a display plane underneath the page
 */
class CWebVideoServer : private CThread
{
public:
  CWebVideoServer();
  ~CWebVideoServer() override;

  bool Start(const std::string& path, std::vector<std::pair<uint32_t, uint64_t>> formats);
  void Stop();
  const std::string& GetPath() const { return m_path; }

  /*!
   * \brief Send the frames to the observer, none with nullptr; any thread
   */
  void SetObserver(IWebVideoObserver* observer);

private:
  void Process() override;
  void Accept();
  bool Receive(const std::shared_ptr<CWebVideoConnection>& connection);
  bool Hello(const CWebVideoConnection& connection);
  void Deliver(const std::shared_ptr<CWebVideoConnection>& connection,
               const kodi_web_video_frame& frame,
               const std::vector<int>& fds);
  void Clear();
  void Place(const CRect& rect);

  std::string m_path;
  int m_listenFd{-1};
  int m_stopFd{-1};
  std::vector<std::pair<uint32_t, uint64_t>> m_formats;
  std::shared_ptr<CWebVideoFramePool> m_pool;
  std::vector<std::shared_ptr<CWebVideoConnection>> m_connections;
  CCriticalSection m_observerLock;
  IWebVideoObserver* m_observer{nullptr};
};

} // namespace WEB
} // namespace KODI

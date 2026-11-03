/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebVideoServer.h"

#include "WebPlayerDefs.h"
#include "kodi-web-video.h"
#include "utils/log.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <mutex>

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

using namespace KODI::WEB;

namespace
{

void Signal(int fd)
{
  if (eventfd_write(fd, 1) < 0)
    CLog::Log(LOGERROR, "CWebVideoServer::{}: cannot signal: {}", __FUNCTION__, strerror(errno));
}

void CloseAll(const std::vector<int>& fds)
{
  for (int fd : fds)
    close(fd);
}

bool FrameIsValid(const kodi_web_video_frame& frame, size_t fdCount)
{
  if (frame.object_count != fdCount || frame.object_count == 0 || frame.plane_count == 0 ||
      frame.plane_count > KODI_WEB_VIDEO_MAX_PLANES)
    return false;

  return std::all_of(frame.object, frame.object + frame.plane_count,
                     [&frame](uint32_t object) { return object < frame.object_count; });
}

} // namespace

CWebVideoConnection::CWebVideoConnection(int fd) : m_fd(fd)
{
}

CWebVideoConnection::~CWebVideoConnection()
{
  close(m_fd);
}

void CWebVideoConnection::Release(uint32_t id)
{
  const kodi_web_video_release release{KODI_WEB_VIDEO_RELEASE, id};
  // a sink that is gone no longer waits for its frames
  send(m_fd, &release, sizeof(release), MSG_NOSIGNAL | MSG_DONTWAIT);
}

CWebVideoFrame::CWebVideoFrame(int id) : CVideoBufferDRMPRIME(id)
{
}

CWebVideoFrame::~CWebVideoFrame()
{
  Clear();
}

void CWebVideoFrame::Set(std::shared_ptr<CWebVideoConnection> connection,
                         const kodi_web_video_frame& frame,
                         const std::vector<int>& fds)
{
  Clear();
  m_connection = std::move(connection);
  m_frameId = frame.id;
  m_width = frame.width;
  m_height = frame.height;
  m_parN = frame.par_n ? frame.par_n : 1;
  m_parD = frame.par_d ? frame.par_d : 1;
  m_matrix = frame.matrix;
  m_range = frame.range;
  m_transfer = frame.transfer;

  m_descriptor = {};
  m_descriptor.nb_objects = static_cast<int>(fds.size());
  for (size_t i = 0; i < fds.size(); ++i)
  {
    m_descriptor.objects[i].fd = fds[i];
    m_descriptor.objects[i].size = static_cast<size_t>(lseek(fds[i], 0, SEEK_END));
    m_descriptor.objects[i].format_modifier = frame.modifier;
  }

  AVDRMLayerDescriptor& layer = m_descriptor.layers[0];
  m_descriptor.nb_layers = 1;
  layer.format = frame.fourcc;
  layer.nb_planes = static_cast<int>(frame.plane_count);
  for (uint32_t i = 0; i < frame.plane_count; ++i)
  {
    layer.planes[i].object_index = static_cast<int>(frame.object[i]);
    layer.planes[i].offset = frame.offset[i];
    layer.planes[i].pitch = frame.stride[i];
  }
}

void CWebVideoFrame::Clear()
{
  for (int i = 0; i < m_descriptor.nb_objects; ++i)
    close(m_descriptor.objects[i].fd);
  m_descriptor = {};

  if (m_connection)
    m_connection->Release(m_frameId);
  m_connection.reset();
}

AVDRMFrameDescriptor* CWebVideoFrame::GetDescriptor() const
{
  return const_cast<AVDRMFrameDescriptor*>(&m_descriptor);
}

CWebVideoFramePool::~CWebVideoFramePool()
{
  for (CWebVideoFrame* frame : m_all)
    delete frame;
}

CVideoBuffer* CWebVideoFramePool::Get()
{
  std::unique_lock lock(m_critSection);

  CWebVideoFrame* frame;
  if (!m_free.empty())
  {
    frame = m_all[m_free.front()];
    m_free.pop_front();
  }
  else
  {
    frame = new CWebVideoFrame(static_cast<int>(m_all.size()));
    m_all.push_back(frame);
  }

  frame->Acquire(GetPtr());
  return frame;
}

void CWebVideoFramePool::Return(int id)
{
  std::unique_lock lock(m_critSection);
  m_all[id]->Clear();
  m_free.push_back(id);
}

CWebVideoServer::CWebVideoServer()
  : CThread("webvideoserver"),
    m_pool(std::make_shared<CWebVideoFramePool>())
{
}

CWebVideoServer::~CWebVideoServer()
{
  Stop();
}

bool CWebVideoServer::Start(const std::string& path,
                            std::vector<std::pair<uint32_t, uint64_t>> formats,
                            std::function<std::string()> dmaHeap)
{
  sockaddr_un addr{};
  if (path.size() >= sizeof(addr.sun_path) || formats.empty())
  {
    CLog::Log(LOGERROR, "CWebVideoServer::{}: cannot serve {} for {} formats", __FUNCTION__, path,
              formats.size());
    return false;
  }
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, path.c_str(), path.size());

  m_listenFd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  m_stopFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  unlink(path.c_str());
  if (m_listenFd < 0 || m_stopFd < 0 ||
      bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
      chmod(path.c_str(), S_IRUSR | S_IWUSR) < 0 || listen(m_listenFd, SOMAXCONN) < 0)
  {
    CLog::Log(LOGERROR, "CWebVideoServer::{}: cannot serve {}: {}", __FUNCTION__, path,
              strerror(errno));
    for (int* fd : {&m_listenFd, &m_stopFd})
    {
      if (*fd >= 0)
        close(*fd);
      *fd = -1;
    }
    return false;
  }

  if (formats.size() > KODI_WEB_VIDEO_MAX_FORMATS)
  {
    CLog::Log(LOGWARNING, "CWebVideoServer::{}: offering the first {} of {} formats", __FUNCTION__,
              KODI_WEB_VIDEO_MAX_FORMATS, formats.size());
    formats.resize(KODI_WEB_VIDEO_MAX_FORMATS);
  }
  m_formats = std::move(formats);
  m_dmaHeap = std::move(dmaHeap);
  m_path = path;
  Create();
  CLog::Log(LOGINFO, "CWebVideoServer::{}: web video at {}, {} formats", __FUNCTION__, path,
            m_formats.size());
  return true;
}

void CWebVideoServer::Stop()
{
  if (m_listenFd < 0)
    return;

  m_bStop = true;
  Signal(m_stopFd);
  StopThread(true);
  m_connections.clear();
  m_placedBy.reset();

  close(m_listenFd);
  close(m_stopFd);
  m_listenFd = -1;
  m_stopFd = -1;
  unlink(m_path.c_str());
}

void CWebVideoServer::SetObserver(IWebVideoObserver* observer)
{
  std::unique_lock lock(m_observerLock);
  m_observer = observer;
}

void CWebVideoServer::Process()
{
  while (!m_bStop)
  {
    std::vector<pollfd> fds{{m_stopFd, POLLIN, 0}, {m_listenFd, POLLIN, 0}};
    for (const auto& connection : m_connections)
      fds.push_back({connection->GetFd(), POLLIN, 0});

    if (poll(fds.data(), fds.size(), -1) < 0)
    {
      if (errno == EINTR)
        continue;
      CLog::Log(LOGERROR, "CWebVideoServer::{}: poll failed: {}", __FUNCTION__, strerror(errno));
      return;
    }

    if (fds[0].revents)
      return;

    std::vector<std::shared_ptr<CWebVideoConnection>> gone;
    for (size_t i = 2; i < fds.size(); ++i)
    {
      if (fds[i].revents && !Receive(m_connections[i - 2]))
        gone.push_back(m_connections[i - 2]);
    }
    if (!gone.empty())
    {
      std::erase_if(m_connections, [&gone](const auto& connection)
                    { return std::ranges::find(gone, connection) != gone.end(); });
      Clear();
      if (std::ranges::find(gone, m_placedBy) != gone.end())
      {
        m_placedBy.reset();
        Place(CRect());
      }
    }

    if (fds[1].revents)
      Accept();
  }
}

void CWebVideoServer::Accept()
{
  const int fd = accept4(m_listenFd, nullptr, nullptr, SOCK_CLOEXEC);
  if (fd < 0)
  {
    CLog::Log(LOGERROR, "CWebVideoServer::{}: accept failed: {}", __FUNCTION__, strerror(errno));
    return;
  }
  m_connections.push_back(std::make_shared<CWebVideoConnection>(fd));
}

bool CWebVideoServer::Hello(const CWebVideoConnection& connection)
{
  kodi_web_video_hello_reply reply{};
  reply.type = KODI_WEB_VIDEO_HELLO_REPLY;
  reply.version = KODI_WEB_VIDEO_VERSION;
  reply.format_count = static_cast<uint32_t>(m_formats.size());
  reply.held_frames = VIDEO_FRAMES_HELD;
  for (size_t i = 0; i < m_formats.size(); ++i)
  {
    reply.formats[i].fourcc = m_formats[i].first;
    reply.formats[i].modifier = m_formats[i].second;
  }
  const std::string heap = m_dmaHeap ? m_dmaHeap() : std::string();
  heap.copy(reply.dma_heap, sizeof(reply.dma_heap) - 1);

  CLog::Log(LOGINFO, "CWebVideoServer::{}: {} formats, {} frames held, dma-heap \"{}\"",
            __FUNCTION__, reply.format_count, reply.held_frames, heap);
  return send(connection.GetFd(), &reply, sizeof(reply), MSG_NOSIGNAL) == sizeof(reply);
}

bool CWebVideoServer::Receive(const std::shared_ptr<CWebVideoConnection>& connection)
{
  alignas(kodi_web_video_frame) uint8_t message[sizeof(kodi_web_video_frame)];
  alignas(cmsghdr) char control[CMSG_SPACE(sizeof(int) * KODI_WEB_VIDEO_MAX_PLANES)];
  iovec iov{message, sizeof(message)};
  msghdr msg{};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  const ssize_t len = recvmsg(connection->GetFd(), &msg, MSG_CMSG_CLOEXEC);
  if (len <= 0)
    return false;

  std::vector<int> fds;
  for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
  {
    if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
      continue;
    const size_t count = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
    const int* received = reinterpret_cast<const int*>(CMSG_DATA(cmsg));
    fds.insert(fds.end(), received, received + count);
  }

  uint32_t type;
  if (static_cast<size_t>(len) < sizeof(type) || (msg.msg_flags & MSG_CTRUNC))
  {
    CloseAll(fds);
    return false;
  }
  memcpy(&type, message, sizeof(type));

  switch (type)
  {
    case KODI_WEB_VIDEO_HELLO:
    {
      kodi_web_video_hello hello;
      CloseAll(fds);
      if (static_cast<size_t>(len) != sizeof(hello))
        return false;
      memcpy(&hello, message, sizeof(hello));
      return hello.version == KODI_WEB_VIDEO_VERSION && Hello(*connection);
    }
    case KODI_WEB_VIDEO_FRAME:
    {
      kodi_web_video_frame frame;
      if (static_cast<size_t>(len) != sizeof(frame))
      {
        CloseAll(fds);
        return false;
      }
      memcpy(&frame, message, sizeof(frame));
      Deliver(connection, frame, fds);
      return true;
    }
    case KODI_WEB_VIDEO_CLEAR:
      CloseAll(fds);
      Clear();
      return true;
    case KODI_WEB_VIDEO_RECT:
    {
      kodi_web_video_rect rect;
      CloseAll(fds);
      if (static_cast<size_t>(len) != sizeof(rect))
        return false;
      memcpy(&rect, message, sizeof(rect));
      if (rect.width < 0 || rect.height < 0)
        return false;
      m_placedBy = connection;
      Place(CRect(static_cast<float>(rect.x), static_cast<float>(rect.y),
                  static_cast<float>(rect.x + rect.width),
                  static_cast<float>(rect.y + rect.height)));
      return true;
    }
    default:
      CloseAll(fds);
      return false;
  }
}

void CWebVideoServer::Deliver(const std::shared_ptr<CWebVideoConnection>& connection,
                              const kodi_web_video_frame& frame,
                              const std::vector<int>& fds)
{
  if (!FrameIsValid(frame, fds.size()))
  {
    CLog::Log(LOGERROR, "CWebVideoServer::{}: frame {} with {} planes in {} of {} objects",
              __FUNCTION__, frame.id, frame.plane_count, fds.size(), frame.object_count);
    CloseAll(fds);
    connection->Release(frame.id);
    return;
  }

  auto* buffer = static_cast<CWebVideoFrame*>(m_pool->Get());
  buffer->Set(connection, frame, fds);

  std::unique_lock lock(m_observerLock);
  if (m_observer)
    m_observer->OnWebVideoFrame(buffer);
  else
    buffer->Release();
}

void CWebVideoServer::Clear()
{
  std::unique_lock lock(m_observerLock);
  if (m_observer)
    m_observer->OnWebVideoClear();
}

void CWebVideoServer::Place(const CRect& rect)
{
  std::unique_lock lock(m_observerLock);
  if (m_observer)
    m_observer->OnWebVideoRect(rect);
}

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/SystemClock.h"
#include "utils/logtypes.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <Platinum/Source/Devices/MediaRenderer/PltMediaController.h>
#include <Platinum/Source/Platinum/Platinum.h>

namespace UPNP
{

class CUPnPPlayerController : public PLT_MediaControllerDelegate
{
public:
  //! Sends one request to the renderer, its reply addressed to the userdata.
  using Request = std::function<NPT_Result(void* userdata)>;

  CUPnPPlayerController(PLT_MediaController* control, PLT_DeviceDataReference& device);
  ~CUPnPPlayerController() override;

  NPT_String GetTransportState() const;

  void OnGetTransportInfoResult(NPT_Result res,
                                PLT_DeviceDataReference& device,
                                PLT_TransportInfo* info,
                                void* userdata) override;

  void UpdatePositionInfo();

  void OnGetPositionInfoResult(NPT_Result res,
                               PLT_DeviceDataReference& device,
                               PLT_PositionInfo* info,
                               void* userdata) override;

  // Platinum identifies a reply only by its userdata pointer, so each action is its own delegate.
  // A wait pumps the render loop through the busy dialog, which can re-enter the player and start
  // another action.
  class CAction : public PLT_MediaControllerDelegate
  {
  public:
    explicit CAction(CUPnPPlayerController& owner) : m_owner(owner) {}

    CEvent& Event() { return m_event; }
    NPT_Result GetStatus() const { return m_status; }
    void Retire() { m_retired = true; }
    bool IsSpent() const { return m_retired && m_replied; }

    PLT_TransportInfo GetTransportInfo() const;
    NPT_String GetTransportState() const;

    void OnSetAVTransportURIResult(NPT_Result res,
                                   PLT_DeviceDataReference& device,
                                   void* userdata) override;
    void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override;
    void OnStopResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override;
    void OnSetNextAVTransportURIResult(NPT_Result res,
                                       PLT_DeviceDataReference& device,
                                       void* userdata) override;
    void OnGetTransportInfoResult(NPT_Result res,
                                  PLT_DeviceDataReference& device,
                                  PLT_TransportInfo* info,
                                  void* userdata) override;

  private:
    void Complete(NPT_Result res, const char* action);

    CUPnPPlayerController& m_owner;
    mutable CCriticalSection m_section;
    PLT_TransportInfo m_trainfo;
    CEvent m_event;
    std::atomic<NPT_Result> m_status{NPT_FAILURE};
    std::atomic<bool> m_replied{false};
    std::atomic<bool> m_retired{false};
  };

  CAction* BeginAction();

  // Not freed here: the caller reads the reply off the action after the wait. A later BeginAction
  // frees it once its reply has arrived.
  void EndAction(CAction& action) { action.Retire(); }

  /*!
   * \brief Send a request as a new action.
   * \param action Set to the action, or nullptr when the request was not sent.
   */
  NPT_Result Send(CAction*& action, const Request& request);

  /*!
   * \brief Send a request and wait for its reply, through the busy dialog.
   * \return The reply's status; a failure if the request was not sent or no reply came.
   */
  NPT_Result Call(const Request& request, XbmcThreads::EndTime<>& timeout);

  //! As Call(), waiting without the busy dialog.
  NPT_Result Call(const Request& request, std::chrono::milliseconds timeout);

  /*!
   * \brief Ask for the transport info and wait for it, through the busy dialog. A failed reply
   * reads as STOPPED with an error status.
   * \return A failure if the request was not sent or no reply came.
   */
  NPT_Result QueryTransport(XbmcThreads::EndTime<>& timeout, PLT_TransportInfo& info);

  Request GetTransportInfo();
  Request Stop();
  Request Play();
  Request SetAVTransportURI(std::string uri, std::string metadata);
  Request SetNextAVTransportURI(std::string uri, std::string metadata);

  size_t HeldActionCount() const;

  NPT_Result WaitForReply(CAction& action, XbmcThreads::EndTime<>& timeout);
  bool WaitForReplyFor(CAction& action, std::chrono::milliseconds timeout);

  PLT_MediaController* m_control;
  PLT_DeviceDataReference m_device;
  NPT_UInt32 m_instance = 0;

  PLT_PositionInfo GetPosition() const;

private:
  // Platinum never replies to a request it did not accept, so waiting for one would hold the action
  // for the life of the player.
  void DiscardUnsent(CAction& action);
  void Release(CAction& action);

  // Platinum fails an accepted request on its own HTTP timeout, so an action normally replies and
  // is freed here. One accepted while the control point is stopping never replies, and is held
  // until the player goes away. Called with m_actionSection held.
  void ReapSpent();

  mutable CCriticalSection m_actionSection;
  std::vector<std::unique_ptr<CAction>> m_actions;

  mutable CCriticalSection m_section;
  PLT_TransportInfo m_trainfo;
  PLT_PositionInfo m_posinfo;
  // Polling starts with the first position reply, which OpenFile asks for.
  bool m_pollOutstanding = true;
  XbmcThreads::EndTime<> m_nextPoll;
  Logger m_logger;
};

} // namespace UPNP

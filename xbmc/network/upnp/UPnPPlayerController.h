/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceBroker.h"
#include "UPnP.h"
#include "dialogs/GUIDialogBusy.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/SystemClock.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/logtypes.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <vector>

#include <Platinum/Source/Devices/MediaRenderer/PltMediaController.h>
#include <Platinum/Source/Platinum/Platinum.h>

namespace UPNP
{

inline NPT_Result WaitOnEvent(CEvent& event, XbmcThreads::EndTime<>& timeout)
{
  if (event.Wait(std::chrono::milliseconds(0)))
    return NPT_SUCCESS;

  if (!CGUIDialogBusy::WaitOnEvent(event))
    return NPT_FAILURE;

  return NPT_SUCCESS;
}

class CUPnPPlayerController : public PLT_MediaControllerDelegate
{
public:
  CUPnPPlayerController(PLT_MediaController* control, PLT_DeviceDataReference& device)
    : m_control(control),
      m_device(device),
      m_posinfo({}),
      m_logger(CServiceBroker::GetLogging().GetLogger("CUPnPPlayerController"))
  {
  }

  NPT_String GetTransportState() const
  {
    std::unique_lock lock(m_section);
    return m_trainfo.cur_transport_state;
  }

  NPT_String GetTransportStatus() const
  {
    std::unique_lock lock(m_section);
    return m_trainfo.cur_transport_status;
  }

  void OnGetTransportInfoResult(NPT_Result res,
                                PLT_DeviceDataReference& device,
                                PLT_TransportInfo* info,
                                void* userdata) override
  {
    std::unique_lock lock(m_section);

    if (NPT_FAILED(res))
    {
      m_logger->error("OnGetTransportInfoResult failed");
      m_trainfo.cur_speed = "0";
      m_trainfo.cur_transport_state = "STOPPED";
      m_trainfo.cur_transport_status = "ERROR_OCCURED";
    }
    else
      m_trainfo = *info;
  }

  void UpdatePositionInfo()
  {
    if (m_postime == 0 || m_postime > CTimeUtils::GetFrameTime())
      return;

    m_control->GetTransportInfo(m_device, m_instance, this);
    m_control->GetPositionInfo(m_device, m_instance, this);
    m_postime = 0;
  }

  void OnGetPositionInfoResult(NPT_Result res,
                               PLT_DeviceDataReference& device,
                               PLT_PositionInfo* info,
                               void* userdata) override
  {
    std::unique_lock lock(m_section);

    if (NPT_FAILED(res) || info == NULL)
    {
      m_logger->error("OnGetPositionInfoResult failed");
      m_posinfo = PLT_PositionInfo();
    }
    else
      m_posinfo = *info;
    m_postime = CTimeUtils::GetFrameTime() + 500;
  }

  ~CUPnPPlayerController() override
  {
    std::unique_lock lock(m_actionSection);
    for (const auto& action : m_actions)
      CUPnP::UnregisterUserdata(action.get());
  }

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

    void OnSetAVTransportURIResult(NPT_Result res,
                                   PLT_DeviceDataReference& device,
                                   void* userdata) override
    {
      Complete(res, "OnSetAVTransportURIResult");
    }

    void OnPlayResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
    {
      Complete(res, "OnPlayResult");
    }

    void OnStopResult(NPT_Result res, PLT_DeviceDataReference& device, void* userdata) override
    {
      Complete(res, "OnStopResult");
    }

    void OnGetTransportInfoResult(NPT_Result res,
                                  PLT_DeviceDataReference& device,
                                  PLT_TransportInfo* info,
                                  void* userdata) override
    {
      m_owner.OnGetTransportInfoResult(res, device, info, userdata);
      Complete(res, "OnGetTransportInfoResult");
    }

  private:
    void Complete(NPT_Result res, const char* action)
    {
      if (NPT_FAILED(res))
        m_owner.m_logger->error("{} failed", action);
      m_status = res;
      m_replied = true;
      m_event.Set();
    }

    CUPnPPlayerController& m_owner;
    CEvent m_event;
    std::atomic<NPT_Result> m_status{NPT_FAILURE};
    std::atomic<bool> m_replied{false};
    std::atomic<bool> m_retired{false};
  };

  CAction* BeginAction()
  {
    std::unique_lock lock(m_actionSection);
    ReapSpent();
    m_actions.push_back(std::make_unique<CAction>(*this));
    CAction* action = m_actions.back().get();
    CUPnP::RegisterUserdata(action);
    return action;
  }

  // Not freed here: the caller reads the reply off the action after the wait. A later BeginAction
  // frees it once its reply has arrived.
  void EndAction(CAction& action) { action.Retire(); }

  // Platinum never replies to a request it did not accept, so waiting for one would hold the action
  // for the life of the player.
  void DiscardUnsent(CAction& action)
  {
    action.Retire();
    Release(action);
  }

  template<typename F>
  NPT_Result Send(CAction*& action, F&& send)
  {
    action = BeginAction();
    const NPT_Result res = send(action);
    if (NPT_FAILED(res))
    {
      DiscardUnsent(*action);
      action = nullptr;
    }
    return res;
  }

  NPT_Result SendGetTransportInfo(CAction*& action)
  {
    return Send(action, [this](void* userdata)
                { return m_control->GetTransportInfo(m_device, m_instance, userdata); });
  }

  NPT_Result SendStop(CAction*& action)
  {
    return Send(action,
                [this](void* userdata) { return m_control->Stop(m_device, m_instance, userdata); });
  }

  NPT_Result SendPlay(CAction*& action)
  {
    return Send(action, [this](void* userdata)
                { return m_control->Play(m_device, m_instance, "1", userdata); });
  }

  NPT_Result SendSetAVTransportURI(CAction*& action, const char* uri, const char* metadata)
  {
    return Send(
        action, [&](void* userdata)
        { return m_control->SetAVTransportURI(m_device, m_instance, uri, metadata, userdata); });
  }

  NPT_Result SendSetNextAVTransportURI(CAction*& action, const char* uri, const char* metadata)
  {
    return Send(action,
                [&](void* userdata)
                {
                  return m_control->SetNextAVTransportURI(m_device, m_instance, uri, metadata,
                                                          userdata);
                });
  }

  size_t HeldActionCount() const
  {
    std::unique_lock lock(m_actionSection);
    return m_actions.size();
  }

  NPT_Result WaitForReply(CAction& action, XbmcThreads::EndTime<>& timeout)
  {
    const NPT_Result result = WaitOnEvent(action.Event(), timeout);
    EndAction(action);
    return result;
  }

  bool WaitForReplyFor(CAction& action, std::chrono::milliseconds timeout)
  {
    const bool replied = action.Event().Wait(timeout);
    EndAction(action);
    return replied;
  }

  PLT_MediaController* m_control;
  PLT_DeviceDataReference m_device;
  NPT_UInt32 m_instance = 0;

  unsigned int m_postime = 0;

  PLT_PositionInfo m_posinfo;

private:
  void Release(CAction& action)
  {
    std::unique_lock lock(m_actionSection);
    CUPnP::UnregisterUserdata(&action);
    std::erase_if(m_actions, [&action](const auto& held) { return held.get() == &action; });
  }

  // Platinum fails an accepted request on its own HTTP timeout, so an action normally replies and
  // is freed here. One accepted while the control point is stopping never replies, and is held
  // until the player goes away. Called with m_actionSection held.
  void ReapSpent()
  {
    const auto spent = [](const std::unique_ptr<CAction>& action)
    {
      if (!action->IsSpent())
        return false;
      CUPnP::UnregisterUserdata(action.get());
      return true;
    };
    std::erase_if(m_actions, spent);
  }

  mutable CCriticalSection m_actionSection;
  std::vector<std::unique_ptr<CAction>> m_actions;

  mutable CCriticalSection m_section;
  PLT_TransportInfo m_trainfo;
  Logger m_logger;
};

} // namespace UPNP

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

class IPlayerCallback;

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
  CUPnPPlayerController(PLT_MediaController* control,
                        PLT_DeviceDataReference& device,
                        IPlayerCallback& callback)
    : m_control(control),
      m_transport(NULL),
      m_device(device),
      m_callback(callback),
      m_posinfo({}),
      m_logger(CServiceBroker::GetLogging().GetLogger("CUPnPPlayerController"))
  {
    m_device->FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", m_transport);
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
    m_posevnt.Set();
  }

  ~CUPnPPlayerController() override
  {
    std::unique_lock lock(m_actionSection);
    for (const auto& action : m_actions)
      CUPnP::UnregisterUserdata(action.get());
  }

  // Platinum identifies a reply only by the userdata pointer it carries, so each action is its own
  // delegate and a reply can reach only the action it belongs to. Actions are per-call because the
  // busy dialog's render loop, pumped while a wait is in progress, can re-enter the player and
  // start another.
  class CAction : public PLT_MediaControllerDelegate
  {
  public:
    explicit CAction(CUPnPPlayerController& owner) : m_owner(owner) {}

    CEvent& Event() { return m_event; }
    NPT_Result GetStatus() const { return m_status; }
    void Retire() { m_retired = true; }
    bool IsSpent() const { return m_retired && m_replied; }

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

    void OnSetAVTransportURIResult(NPT_Result res,
                                   PLT_DeviceDataReference& device,
                                   void* userdata) override
    {
      Complete(res, "OnSetAVTransportURIResult");
    }

    void OnSetNextAVTransportURIResult(NPT_Result res,
                                       PLT_DeviceDataReference& device,
                                       void* userdata) override
    {
      Complete(res, "OnSetNextAVTransportURIResult");
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
      {
        std::unique_lock lock(m_section);
        if (NPT_FAILED(res) || info == NULL)
        {
          m_trainfo.cur_speed = "0";
          m_trainfo.cur_transport_state = "STOPPED";
          m_trainfo.cur_transport_status = "ERROR_OCCURED";
        }
        else
          m_trainfo = *info;
      }
      // The controller's copy is what CUPnPPlayer::Process watches for the end of playback, and
      // the only other thing writing it is a poll up to 500ms behind. Leaving it stale here ends
      // playback the moment it starts.
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
    mutable CCriticalSection m_section;
    PLT_TransportInfo m_trainfo;
    CEvent m_event;
    std::atomic<NPT_Result> m_status{NPT_FAILURE};
    std::atomic<bool> m_replied{false};
    std::atomic<bool> m_retired{false};
  };

  // The returned action belongs to the caller until it reaches EndAction; the following
  // BeginAction is what frees it.
  CAction* BeginAction()
  {
    std::unique_lock lock(m_actionSection);
    ReapSpent();
    m_actions.push_back(std::make_unique<CAction>(*this));
    CAction* action = m_actions.back().get();
    CUPnP::RegisterUserdata(action);
    return action;
  }

  // Retires an action without freeing it: the caller reads the reply off it after the wait, and
  // the next BeginAction is what releases it.
  void EndAction(CAction& action) { action.Retire(); }

  // Platinum answers nothing it did not accept, so an action whose request never left will never
  // reply and would otherwise sit in the list for the life of the player. The caller is handed a
  // null pointer for it, so nothing reads it after this.
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

  // How many actions are held, whether outstanding or waiting on a reply that retires them.
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
  PLT_Service* m_transport;
  PLT_DeviceDataReference m_device;
  NPT_UInt32 m_instance = 0;
  IPlayerCallback& m_callback;

  unsigned int m_postime = 0;

  CEvent m_posevnt;
  PLT_PositionInfo m_posinfo;

  PLT_TransportInfo m_trainfo;

private:
  void Release(CAction& action)
  {
    std::unique_lock lock(m_actionSection);
    CUPnP::UnregisterUserdata(&action);
    std::erase_if(m_actions, [&action](const auto& held) { return held.get() == &action; });
  }

  // Platinum fails an accepted request on its own HTTP timeout when the renderer never answers, so
  // an action ordinarily replies and is released here. One accepted while the control point is
  // stopping is never answered and is held until the player goes away. The caller holds
  // m_actionSection.
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
  Logger m_logger;
};

} // namespace UPNP

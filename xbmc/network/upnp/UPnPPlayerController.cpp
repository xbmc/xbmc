/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UPnPPlayerController.h"

#include "ServiceBroker.h"
#include "UPnP.h"
#include "dialogs/GUIDialogBusy.h"
#include "utils/log.h"

#include <mutex>
#include <utility>

namespace UPNP
{

namespace
{
//! NPT_ERROR_TIMEOUT when the deadline passes, NPT_FAILURE when the user cancels.
NPT_Result WaitOnEvent(CEvent& event, XbmcThreads::EndTime<>& timeout)
{
  if (event.Wait(std::chrono::milliseconds(0)))
    return NPT_SUCCESS;

  switch (CGUIDialogBusy::WaitOnEventFor(event, timeout.GetTimeLeft()))
  {
    case CGUIDialogBusy::WaitResult::COMPLETED:
      return NPT_SUCCESS;
    case CGUIDialogBusy::WaitResult::TIMED_OUT:
      return NPT_ERROR_TIMEOUT;
    default:
      return NPT_FAILURE;
  }
}

PLT_TransportInfo TransportInfoOf(NPT_Result res, const PLT_TransportInfo* info)
{
  if (NPT_SUCCEEDED(res) && info)
    return *info;

  PLT_TransportInfo failed;
  failed.cur_speed = "0";
  failed.cur_transport_state = "STOPPED";
  failed.cur_transport_status = "ERROR_OCCURED";
  return failed;
}
} // unnamed namespace

CUPnPPlayerController::CUPnPPlayerController(PLT_MediaController* control,
                                             PLT_DeviceDataReference& device)
  : m_control(control),
    m_device(device),
    m_posinfo({}),
    m_logger(CServiceBroker::GetLogging().GetLogger("CUPnPPlayerController"))
{
}

CUPnPPlayerController::~CUPnPPlayerController()
{
  std::unique_lock lock(m_actionSection);
  for (const auto& action : m_actions)
    CUPnP::UnregisterUserdata(action.get());
}

NPT_String CUPnPPlayerController::GetTransportState() const
{
  std::unique_lock lock(m_section);
  return m_trainfo.cur_transport_state;
}

void CUPnPPlayerController::OnGetTransportInfoResult(NPT_Result res,
                                                     PLT_DeviceDataReference& device,
                                                     PLT_TransportInfo* info,
                                                     void* userdata)
{
  if (NPT_FAILED(res))
    m_logger->error("OnGetTransportInfoResult failed");

  std::unique_lock lock(m_section);
  m_trainfo = TransportInfoOf(res, info);
}

void CUPnPPlayerController::UpdatePositionInfo()
{
  {
    std::unique_lock lock(m_section);
    if (m_pollOutstanding || !m_nextPoll.IsTimePast())
      return;
    // Set before sending, because the reply that clears it can arrive before these return.
    m_pollOutstanding = true;
  }

  m_control->GetTransportInfo(m_device, m_instance, this);
  if (NPT_FAILED(m_control->GetPositionInfo(m_device, m_instance, this)))
  {
    std::unique_lock lock(m_section);
    m_pollOutstanding = false;
    m_nextPoll.Set(std::chrono::milliseconds(500));
  }
}

void CUPnPPlayerController::OnGetPositionInfoResult(NPT_Result res,
                                                    PLT_DeviceDataReference& device,
                                                    PLT_PositionInfo* info,
                                                    void* userdata)
{
  std::unique_lock lock(m_section);

  if (NPT_FAILED(res) || info == NULL)
  {
    m_logger->error("OnGetPositionInfoResult failed");
    m_posinfo = PLT_PositionInfo();
  }
  else
    m_posinfo = *info;
  m_pollOutstanding = false;
  m_nextPoll.Set(std::chrono::milliseconds(500));
}

PLT_TransportInfo CUPnPPlayerController::CAction::GetTransportInfo() const
{
  std::unique_lock lock(m_section);
  return m_trainfo;
}

NPT_String CUPnPPlayerController::CAction::GetTransportState() const
{
  std::unique_lock lock(m_section);
  return m_trainfo.cur_transport_state;
}

void CUPnPPlayerController::CAction::OnSetAVTransportURIResult(NPT_Result res,
                                                               PLT_DeviceDataReference& device,
                                                               void* userdata)
{
  Complete(res, "OnSetAVTransportURIResult");
}

void CUPnPPlayerController::CAction::OnPlayResult(NPT_Result res,
                                                  PLT_DeviceDataReference& device,
                                                  void* userdata)
{
  Complete(res, "OnPlayResult");
}

void CUPnPPlayerController::CAction::OnStopResult(NPT_Result res,
                                                  PLT_DeviceDataReference& device,
                                                  void* userdata)
{
  Complete(res, "OnStopResult");
}

void CUPnPPlayerController::CAction::OnSetNextAVTransportURIResult(NPT_Result res,
                                                                   PLT_DeviceDataReference& device,
                                                                   void* userdata)
{
  Complete(res, "OnSetNextAVTransportURIResult");
}

void CUPnPPlayerController::CAction::OnGetTransportInfoResult(NPT_Result res,
                                                              PLT_DeviceDataReference& device,
                                                              PLT_TransportInfo* info,
                                                              void* userdata)
{
  {
    std::unique_lock lock(m_section);
    m_trainfo = TransportInfoOf(res, info);
  }
  // CUPnPPlayer::Process watches the controller's copy for the end of playback. Left to the
  // poll, which can be 500ms behind, it still reads STOPPED as playback starts.
  m_owner.OnGetTransportInfoResult(res, device, info, userdata);
  Complete(res, "OnGetTransportInfoResult");
}

void CUPnPPlayerController::CAction::Complete(NPT_Result res, const char* action)
{
  if (NPT_FAILED(res))
    m_owner.m_logger->error("{} failed", action);
  m_status = res;
  m_replied = true;
  m_event.Set();
}

CUPnPPlayerController::CAction* CUPnPPlayerController::BeginAction()
{
  std::unique_lock lock(m_actionSection);
  ReapSpent();
  m_actions.push_back(std::make_unique<CAction>(*this));
  CAction* action = m_actions.back().get();
  CUPnP::RegisterUserdata(action);
  return action;
}

NPT_Result CUPnPPlayerController::Send(CAction*& action, const Request& request)
{
  action = BeginAction();
  const NPT_Result res = request(action);
  if (NPT_FAILED(res))
  {
    DiscardUnsent(*action);
    action = nullptr;
  }
  return res;
}

NPT_Result CUPnPPlayerController::Call(const Request& request, XbmcThreads::EndTime<>& timeout)
{
  CAction* action = nullptr;
  NPT_CHECK(Send(action, request));
  NPT_CHECK(WaitForReply(*action, timeout));
  return action->GetStatus();
}

NPT_Result CUPnPPlayerController::Call(const Request& request, std::chrono::milliseconds timeout)
{
  CAction* action = nullptr;
  NPT_CHECK(Send(action, request));
  if (!WaitForReplyFor(*action, timeout))
    return NPT_FAILURE;
  return action->GetStatus();
}

NPT_Result CUPnPPlayerController::QueryTransport(XbmcThreads::EndTime<>& timeout,
                                                 PLT_TransportInfo& info)
{
  CAction* action = nullptr;
  NPT_CHECK(Send(action, GetTransportInfo()));
  NPT_CHECK(WaitForReply(*action, timeout));
  info = action->GetTransportInfo();
  return NPT_SUCCESS;
}

CUPnPPlayerController::Request CUPnPPlayerController::GetTransportInfo()
{
  return [this](void* userdata)
  { return m_control->GetTransportInfo(m_device, m_instance, userdata); };
}

CUPnPPlayerController::Request CUPnPPlayerController::Stop()
{
  return [this](void* userdata) { return m_control->Stop(m_device, m_instance, userdata); };
}

CUPnPPlayerController::Request CUPnPPlayerController::Play()
{
  return [this](void* userdata) { return m_control->Play(m_device, m_instance, "1", userdata); };
}

CUPnPPlayerController::Request CUPnPPlayerController::SetAVTransportURI(std::string uri,
                                                                        std::string metadata)
{
  return [this, uri = std::move(uri), metadata = std::move(metadata)](void* userdata)
  {
    return m_control->SetAVTransportURI(m_device, m_instance, uri.c_str(), metadata.c_str(),
                                        userdata);
  };
}

CUPnPPlayerController::Request CUPnPPlayerController::SetNextAVTransportURI(std::string uri,
                                                                            std::string metadata)
{
  return [this, uri = std::move(uri), metadata = std::move(metadata)](void* userdata)
  {
    return m_control->SetNextAVTransportURI(m_device, m_instance, uri.c_str(), metadata.c_str(),
                                            userdata);
  };
}

size_t CUPnPPlayerController::HeldActionCount() const
{
  std::unique_lock lock(m_actionSection);
  return m_actions.size();
}

NPT_Result CUPnPPlayerController::WaitForReply(CAction& action, XbmcThreads::EndTime<>& timeout)
{
  const NPT_Result result = WaitOnEvent(action.Event(), timeout);
  EndAction(action);
  return result;
}

bool CUPnPPlayerController::WaitForReplyFor(CAction& action, std::chrono::milliseconds timeout)
{
  const bool replied = action.Event().Wait(timeout);
  EndAction(action);
  return replied;
}

PLT_PositionInfo CUPnPPlayerController::GetPosition() const
{
  std::unique_lock lock(m_section);
  return m_posinfo;
}

void CUPnPPlayerController::DiscardUnsent(CAction& action)
{
  action.Retire();
  Release(action);
}

void CUPnPPlayerController::Release(CAction& action)
{
  std::unique_lock lock(m_actionSection);
  CUPnP::UnregisterUserdata(&action);
  std::erase_if(m_actions, [&action](const auto& held) { return held.get() == &action; });
}

void CUPnPPlayerController::ReapSpent()
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

} // namespace UPNP

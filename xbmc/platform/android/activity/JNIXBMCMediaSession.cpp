/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCMediaSession.h"

#include "AndroidKey.h"
#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "XBMCApp.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"

#include <androidjni/Context.h>
#include <androidjni/KeyEvent.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCMediaSession";

CJNIXBMCMediaSession::CJNIXBMCMediaSession()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCMediaSession::CJNIXBMCMediaSession(const CJNIXBMCMediaSession& other)
  : CJNIBase(other)
{
  add_instance(m_object, this);
}

CJNIXBMCMediaSession::~CJNIXBMCMediaSession()
{
  remove_instance(this);
}

void CJNIXBMCMediaSession::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onPlayRequested", "()V", (void*)&CJNIXBMCMediaSession::_onPlayRequested},
      {"_onPauseRequested", "()V", (void*)&CJNIXBMCMediaSession::_onPauseRequested},
      {"_onNextRequested", "()V", (void*)&CJNIXBMCMediaSession::_onNextRequested},
      {"_onPreviousRequested", "()V", (void*)&CJNIXBMCMediaSession::_onPreviousRequested},
      {"_onForwardRequested", "()V", (void*)&CJNIXBMCMediaSession::_onForwardRequested},
      {"_onRewindRequested", "()V", (void*)&CJNIXBMCMediaSession::_onRewindRequested},
      {"_onStopRequested", "()V", (void*)&CJNIXBMCMediaSession::_onStopRequested},
      {"_onSeekRequested", "(J)V", (void*)&CJNIXBMCMediaSession::_onSeekRequested},
      {"_onMediaButtonEvent", "(Landroid/content/Intent;)Z", (void*)&CJNIXBMCMediaSession::_onMediaButtonEvent},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCMediaSession::activate(bool state)
{
  if (state == m_isActive)
    return;

  call_method<void>(m_object,
                    "activate", "(Z)V",
                    (jboolean)state);
  m_isActive = state;
}

void CJNIXBMCMediaSession::updatePlaybackState(const CJNIPlaybackState& state)
{
  call_method<void>(m_object,
                    "updatePlaybackState", "(Landroid/media/session/PlaybackState;)V",
                    state.get_raw());
}

void CJNIXBMCMediaSession::updateMetadata(const CJNIMediaMetadata& myData)
{
  call_method<void>(m_object,
                    "updateMetadata", "(Landroid/media/MediaMetadata;)V",
                    myData.get_raw());
}

void CJNIXBMCMediaSession::updateIntent(const CJNIIntent& intent)
{
  call_method<void>(m_object,
                    "updateIntent", "(Landroid/content/Intent;)V",
                    intent.get_raw());
}

void CJNIXBMCMediaSession::OnPlayRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
  {
    if (appPlayer->IsPaused())
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                 static_cast<void*>(new CAction(ACTION_PAUSE)));
  }
}

void CJNIXBMCMediaSession::OnPauseRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
  {
    if (!appPlayer->IsPaused())
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                 static_cast<void*>(new CAction(ACTION_PAUSE)));
  }
}

void CJNIXBMCMediaSession::OnNextRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(ACTION_NEXT_ITEM)));
}

void CJNIXBMCMediaSession::OnPreviousRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(ACTION_PREV_ITEM)));
}

void CJNIXBMCMediaSession::OnForwardRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
  {
    if (!appPlayer->IsPaused())
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_GUI_ACTION, WINDOW_INVALID, -1,
          static_cast<void*>(new CAction(ACTION_PLAYER_FORWARD)));
  }
}

void CJNIXBMCMediaSession::OnRewindRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
  {
    if (!appPlayer->IsPaused())
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_GUI_ACTION, WINDOW_INVALID, -1,
          static_cast<void*>(new CAction(ACTION_PLAYER_REWIND)));
  }
}

void CJNIXBMCMediaSession::OnStopRequested()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                               static_cast<void*>(new CAction(ACTION_STOP)));
}

void CJNIXBMCMediaSession::OnSeekRequested(int64_t pos)
{
  g_application.SeekTime(pos / 1000.0);
}

bool CJNIXBMCMediaSession::OnMediaButtonEvent(const CJNIIntent& intent)
{
  if (CXBMCApp::Get().HasFocus())
  {
    CXBMCApp::Get().onReceive(intent);
    return true;
  }
  return false;
}

bool CJNIXBMCMediaSession::isActive() const
{
  return m_isActive;
}

/**********************/

void CJNIXBMCMediaSession::_onPlayRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnPlayRequested();
}

void CJNIXBMCMediaSession::_onPauseRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnPauseRequested();
}

void CJNIXBMCMediaSession::_onNextRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnNextRequested();
}

void CJNIXBMCMediaSession::_onPreviousRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnPreviousRequested();
}

void CJNIXBMCMediaSession::_onForwardRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnForwardRequested();
}

void CJNIXBMCMediaSession::_onRewindRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnRewindRequested();
}

void CJNIXBMCMediaSession::_onStopRequested(JNIEnv* env, jobject thiz)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnStopRequested();
}

void CJNIXBMCMediaSession::_onSeekRequested(JNIEnv* env, jobject thiz, jlong pos)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    inst->OnSeekRequested(pos);
}

bool CJNIXBMCMediaSession::_onMediaButtonEvent(JNIEnv* env, jobject thiz, jobject intent)
{
  (void)env;

  CJNIXBMCMediaSession *inst = find_instance(thiz);
  if (inst)
    return inst->OnMediaButtonEvent(CJNIIntent(jhobject::fromJNI(intent)));
  return false;
}


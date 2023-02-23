/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireNode.h"

#include "PipewireContext.h"
#include "PipewireCore.h"
#include "PipewireRegistry.h"
#include "PipewireThreadLoop.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <spa/param/format.h>
#include <spa/pod/iter.h>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireNode::CPipewireNode(CPipewireRegistry& registry, uint32_t id, const char* type)
  : CPipewireProxy(registry, id, type, PW_VERSION_NODE), m_nodeEvents(CreateNodeEvents())
{
  pw_proxy_add_object_listener(m_proxy.get(), &m_objectListener, &m_nodeEvents, this);
}

CPipewireNode::~CPipewireNode()
{
  spa_hook_remove(&m_objectListener);
}

void CPipewireNode::EnumerateFormats()
{
  if (!m_info)
    return;

  for (uint32_t param = 0; param < m_info->n_params; param++)
  {
    if (m_info->params[param].id == SPA_PARAM_EnumFormat)
      pw_node_enum_params(m_proxy.get(), 0, m_info->params[param].id, 0, 0, NULL);
  }
}

void CPipewireNode::Info(void* userdata, const struct pw_node_info* info)
{
  auto& node = *reinterpret_cast<CPipewireNode*>(userdata);

  if (node.m_info)
  {
    CLog::Log(LOGDEBUG, "CPipewireNode::{} - node {} changed", __FUNCTION__, info->id);
    pw_node_info* m_info = node.m_info.get();
    m_info = pw_node_info_update(m_info, info);
  }
  else
  {
    node.m_info.reset(pw_node_info_update(node.m_info.get(), info));
  }
}

template<typename T>
static T Parse(uint32_t type, void* body, uint32_t size)
{
  switch (type)
  {
    case SPA_TYPE_Id:
    case SPA_TYPE_Int:
      return *reinterpret_cast<T*>(body);

    default:
      throw std::runtime_error(StringUtils::Format("unhandled type: {}", type));
  }
}

template<typename T>
static std::set<T> ParseArray(uint32_t type, void* body, uint32_t size)
{
  switch (type)
  {
    case SPA_TYPE_Id:
    case SPA_TYPE_Int:
    {
      std::set<T> values;
      values.emplace(Parse<T>(type, body, size));

      return values;
    }
    case SPA_TYPE_Array:
    {
      auto array = reinterpret_cast<spa_pod_array_body*>(body);
      void* p;
      std::set<T> values;
      SPA_POD_ARRAY_BODY_FOREACH(array, size, p)
      values.emplace(Parse<T>(array->child.type, p, array->child.size));

      return values;
    }
    case SPA_TYPE_Choice:
    {
      auto choice = reinterpret_cast<spa_pod_choice_body*>(body);
      void* p;
      std::set<T> values;
      SPA_POD_CHOICE_BODY_FOREACH(choice, size, p)
      values.emplace(Parse<T>(choice->child.type, p, choice->child.size));

      return values;
    }
    default:
      throw std::runtime_error(StringUtils::Format("unhandled array: {}", type));
  }
}

void CPipewireNode::Parse(uint32_t type, void* body, uint32_t size)
{
  switch (type)
  {
    case SPA_TYPE_Object:
    {
      auto object = reinterpret_cast<spa_pod_object_body*>(body);

      switch (object->type)
      {
        case SPA_TYPE_OBJECT_Format:
        {
          spa_pod_prop* prop;
          SPA_POD_OBJECT_BODY_FOREACH(object, size, prop)
          {
            spa_format format = static_cast<spa_format>(prop->key);

            switch (format)
            {
              case SPA_FORMAT_AUDIO_format:
              {
                m_formats = ParseArray<spa_audio_format>(
                    prop->value.type, SPA_POD_CONTENTS(spa_pod_prop, prop), prop->value.size);
                break;
              }
              case SPA_FORMAT_AUDIO_rate:
              {
                m_rates = ParseArray<uint32_t>(
                    prop->value.type, SPA_POD_CONTENTS(spa_pod_prop, prop), prop->value.size);
                break;
              }
              case SPA_FORMAT_AUDIO_position:
              {
                m_channels = ParseArray<spa_audio_channel>(
                    prop->value.type, SPA_POD_CONTENTS(spa_pod_prop, prop), prop->value.size);
                break;
              }
              case SPA_FORMAT_AUDIO_iec958Codec:
              {
                m_iec958Codecs = ParseArray<spa_audio_iec958_codec>(
                    prop->value.type, SPA_POD_CONTENTS(spa_pod_prop, prop), prop->value.size);
                break;
              }
              default:
                break;
            }
          }

          break;
        }
        default:
          return;
      }

      break;
    }
    default:
      return;
  }
}

void CPipewireNode::Param(void* userdata,
                          int seq,
                          uint32_t id,
                          uint32_t index,
                          uint32_t next,
                          const struct spa_pod* param)
{
  auto& node = *reinterpret_cast<CPipewireNode*>(userdata);
  auto& loop = node.GetRegistry().GetCore().GetContext().GetThreadLoop();

  node.Parse(SPA_POD_TYPE(param), SPA_POD_BODY(param), SPA_POD_BODY_SIZE(param));

  loop.Signal(false);
}

pw_node_events CPipewireNode::CreateNodeEvents()
{
  pw_node_events nodeEvents = {};
  nodeEvents.version = PW_VERSION_NODE_EVENTS;
  nodeEvents.info = Info;
  nodeEvents.param = Param;

  return nodeEvents;
}

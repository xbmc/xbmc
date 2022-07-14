/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <mutex>

namespace KODI
{

using RegistrationIdentifierType = int;

class ISignalHandlerData
{
protected:
  ~ISignalHandlerData() = default;

public:
  virtual void Unregister(RegistrationIdentifierType id) = 0;
};

class CSignalRegistration
{
  std::weak_ptr<ISignalHandlerData> m_list;
  RegistrationIdentifierType m_registration;

  template<typename ManagedT>
  friend class CSignalHandlerList;

  CSignalRegistration(std::shared_ptr<ISignalHandlerData> const& list, RegistrationIdentifierType registration)
  : m_list{list}, m_registration{registration}
  {
  }

  CSignalRegistration(CSignalRegistration const& other) = delete;
  CSignalRegistration& operator=(CSignalRegistration const& other) = delete;

public:
  CSignalRegistration() noexcept = default;

  CSignalRegistration(CSignalRegistration&& other) noexcept
  {
    *this = std::move(other);
  }

  inline CSignalRegistration& operator=(CSignalRegistration&& other) noexcept
  {
    Unregister();
    std::swap(m_list, other.m_list);
    m_registration = other.m_registration;
    return *this;
  }

  ~CSignalRegistration() noexcept
  {
    Unregister();
  }

  inline void Unregister()
  {
    if (auto list = m_list.lock())
    {
      list->Unregister(m_registration);
      list.reset();
    }
  }
};

template<typename ManagedT>
class CSignalHandlerList
{
  /**
   * Internal storage for handler list
   *
   * Extra struct so memory handling with shared_ptr and weak_ptr can be done
   * on this level
   */
  struct Data final : public ISignalHandlerData
  {
    CCriticalSection m_handlerCriticalSection;
    std::map<RegistrationIdentifierType, ManagedT> m_handlers;

    void Unregister(RegistrationIdentifierType id) override
    {
      std::unique_lock<CCriticalSection> lock(m_handlerCriticalSection);
      m_handlers.erase(id);
    }
  };

  std::shared_ptr<Data> m_data;
  RegistrationIdentifierType m_lastRegistrationId{};

  CSignalHandlerList(CSignalHandlerList const& other) = delete;
  CSignalHandlerList& operator=(CSignalHandlerList const& other) = delete;

public:
  CSignalHandlerList()
  : m_data{new Data}
  {}

  CSignalRegistration Register(ManagedT const& handler)
  {
    std::unique_lock<CCriticalSection> lock(m_data->m_handlerCriticalSection);
    bool inserted{false};
    while(!inserted)
    {
      inserted = m_data->m_handlers.emplace(++m_lastRegistrationId, handler).second;
    }
    return {m_data, m_lastRegistrationId};
  }

  /**
   * Invoke all registered signal handlers with the provided arguments
   * when the signal type is a std::function or otherwise implements
   * operator()
   */
  template<typename... ArgsT>
  void Invoke(ArgsT&&... args)
  {
    std::unique_lock<CCriticalSection> lock(m_data->m_handlerCriticalSection);
    for (auto const& handler : *this)
    {
      handler.second(std::forward<ArgsT>(args)...);
    }
  }

  auto begin() const { return m_data->m_handlers.cbegin(); }

  auto end() const { return m_data->m_handlers.cend(); }

  /**
   * Get critical section for accessing the handler list
   * \note You must lock this yourself if you iterate through the handler
   * list manually without using \ref Invoke or similar.
   */
  CCriticalSection const& CriticalSection() const
  {
    return m_data->m_handlerCriticalSection;
  }
};

}

/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>

//! \brief A generic container for components.
//! \details A component has to be derived from the BaseType.
//!          Only a single instance of each derived type can be registered.
//!          Intended use is through inheritance.
template<class BaseType>
class CComponentContainer
{
public:
  //! \brief Obtain a component.
  template<class T>
  std::shared_ptr<T> GetComponent()
  {
    return std::const_pointer_cast<T>(std::as_const(*this).template GetComponent<T>());
  }

  //! \brief Obtain a component.
  template<class T>
  std::shared_ptr<const T> GetComponent() const
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    const auto it = m_components.find(std::type_index(typeid(T)));
    if (it != m_components.end())
      return std::static_pointer_cast<const T>((*it).second);

    throw std::logic_error("ComponentContainer: Attempt to obtain non-existent component");
  }

  //! \brief Returns number of registered components.
  std::size_t size() const { return m_components.size(); }

protected:
  //! \brief Register a new component instance.
  void RegisterComponent(const std::shared_ptr<BaseType>& component)
  {
    if (!component)
      return;

    // Note: Extra var needed to avoid clang warning
    // "Expression with side effects will be evaluated despite being used as an operand to 'typeid'"
    // https://stackoverflow.com/questions/46494928/clang-warning-on-expression-side-effects
    const auto& componentRef = *component;

    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_components.insert({std::type_index(typeid(componentRef)), component});
  }

  //! \brief Deregister a component.
  void DeregisterComponent(const std::type_info& typeInfo)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_components.erase(typeInfo);
  }

private:
  mutable CCriticalSection m_critSection; //!< Critical section for map updates
  std::unordered_map<std::type_index, std::shared_ptr<BaseType>>
      m_components; //!< Map of components
};

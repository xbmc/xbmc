/*
 *  Copyright (C) 2010-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <memory>
#include <string>

#include <pipewire/properties.h>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireNode;

struct PipewirePropertiesDeleter
{
  void operator()(pw_properties* p) { pw_properties_free(p); }
};

class CPipewireGlobal
{
public:
  CPipewireGlobal() = default;
  ~CPipewireGlobal();

  CPipewireGlobal& SetName(const std::string& name)
  {
    m_name = name;
    return *this;
  }

  std::string GetName() const { return m_name; }

  CPipewireGlobal& SetDescription(const std::string& description)
  {
    m_description = description;
    return *this;
  }

  std::string GetDescription() const { return m_description; }

  CPipewireGlobal& SetID(uint32_t id)
  {
    m_id = id;
    return *this;
  }

  uint32_t GetID() const { return m_id; }

  CPipewireGlobal& SetPermissions(uint32_t permissions)
  {
    m_permissions = permissions;
    return *this;
  }

  uint32_t GetPermissions() const { return m_permissions; }

  CPipewireGlobal& SetType(const std::string& type)
  {
    m_type = type;
    return *this;
  }

  std::string GetType() const { return m_type; }

  CPipewireGlobal& SetVersion(uint32_t version)
  {
    m_version = version;
    return *this;
  }

  uint32_t GetVersion() const { return m_version; }

  CPipewireGlobal& SetProperties(
      std::unique_ptr<pw_properties, PipewirePropertiesDeleter> properties)
  {
    m_properties = std::move(properties);
    return *this;
  }

  const pw_properties& GetProperties() const { return *m_properties; }

  CPipewireGlobal& SetNode(std::unique_ptr<CPipewireNode> node)
  {
    m_node = std::move(node);
    return *this;
  }

  CPipewireNode& GetNode() const { return *m_node; }

private:
  std::string m_name;
  std::string m_description;
  uint32_t m_id;
  uint32_t m_permissions;
  std::string m_type;
  uint32_t m_version;
  std::unique_ptr<pw_properties, PipewirePropertiesDeleter> m_properties;
  std::unique_ptr<CPipewireNode> m_node;
};

} // namespace PIPEWIRE
} // namespace KODI

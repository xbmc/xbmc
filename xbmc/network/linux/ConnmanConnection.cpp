/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ConnmanConnection.h"

#ifdef HAS_DBUS
#include "linux/DBusUtil.h"
#include "linux/DBusMessage.h"
#include "utils/log.h"

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CConnmanConnection::CConnmanConnection(const char *serviceObject)
{
  m_serviceObject = serviceObject;

  // should use CONNMAN_SERVICE here (in connman/dbus.h)
  CDBusMessage message("net.connman", serviceObject, "net.connman.Service", "GetProperties");
  CDBusReplyPtr reply = message.SendSystem();
  m_properties = reply->GetNextArgument();

  UpdateConnection();

  dbus_error_init (&m_error);

  // TODO: do not use dbus_connection_pop_message() that requires the use of a
  // private connection
  m_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &m_error);
  if (m_connection)
  {
    dbus_connection_set_exit_on_disconnect(m_connection, false);

    dbus_bus_add_match(m_connection, "type='signal',interface='net.connman.Service'", &m_error);
    dbus_connection_flush(m_connection);
    if (dbus_error_is_set(&m_error))
    {
      CLog::Log(LOGERROR, "ConnmanConnection: %s Failed to attach to signal %s", serviceObject, m_error.message);
      dbus_connection_close(m_connection);
      dbus_connection_unref(m_connection);
      m_connection = NULL;
    }
  }
  else
    CLog::Log(LOGERROR, "ConnmanConnection: %s Failed to get a DBus connection %s", serviceObject, m_error.message);
}

CConnmanConnection::~CConnmanConnection()
{
  if (m_connection)
  {
    dbus_connection_close(m_connection);
    dbus_connection_unref(m_connection);
    m_connection = NULL;
  }

  dbus_error_free (&m_error);
}

std::string CConnmanConnection::GetName() const
{
  return m_name;
}

std::string CConnmanConnection::GetAddress() const
{
  return m_address;
}

std::string CConnmanConnection::GetNetmask() const
{
  return m_netmask;
}

std::string CConnmanConnection::GetGateway() const
{
  return m_gateway;
}

std::string CConnmanConnection::GetNameServer() const
{
  return "127.0.0.1";
}

std::string CConnmanConnection::GetMacAddress() const
{
  return m_macaddress;
}

ConnectionType CConnmanConnection::GetType() const
{
  return m_type;
}

ConnectionState CConnmanConnection::GetState() const
{
  return m_state;
}

unsigned int CConnmanConnection::GetSpeed() const
{
  return m_speed;
}

IPConfigMethod CConnmanConnection::GetMethod() const
{
  if (m_method.find("dhcp") != std::string::npos)
    return IP_CONFIG_DHCP;
  else if (m_method.find("manual") != std::string::npos)
    return IP_CONFIG_STATIC;
  else
    return IP_CONFIG_DISABLED;
}

unsigned int CConnmanConnection::GetStrength() const
{
  return m_strength;;
}

EncryptionType CConnmanConnection::GetEncryption() const
{
  return m_encryption;
}

bool CConnmanConnection::Connect(IPassphraseStorage *storage, const CIPConfig &ipconfig)
{
  if (m_encryption != NETWORK_CONNECTION_ENCRYPTION_NONE)
  {
    if (!storage->GetPassphrase(m_serviceObject, m_passphrase))
      return false;

    CDBusMessage message("net.connman", m_serviceObject.c_str(), "net.connman.Service", "SetProperty");
    message.AppendArgument("Passphrase");
    message.AppendArgument(m_passphrase.c_str());

    CDBusReplyPtr reply = message.SendSystem();
    if (reply->IsErrorSet())
    {
      CLog::Log(LOGERROR, "ConnmanConnection: Failed to set passphrase");
      return false;
    }
  }

  CLog::Log(LOGDEBUG, "CConnmanConnection::Connect:m_serviceObject(%s)", m_serviceObject.c_str());
  CDBusMessage message("net.connman", m_serviceObject.c_str(), "net.connman.Service", "Connect");
  return message.SendAsyncSystem();
}

//-----------------------------------------------------------------------
bool CConnmanConnection::PumpNetworkEvents()
{
  bool result = false;

  if (m_connection)
  {
    dbus_connection_read_write(m_connection, 0);
    DBusMessage *msg = dbus_connection_pop_message(m_connection);

    if (msg)
    {
      CDBusReplyPtr reply = CDBusReplyPtr(new CDBusReply(msg));

      if (dbus_message_is_signal(msg, "net.connman.Service", "PropertyChanged"))
      {
        CVariant key = reply->GetNextArgument();
        m_properties[key.asString()] = reply->GetNextArgument();

        UpdateConnection();
        result = true;
      }

      dbus_message_unref(msg);
    }
  }

  return result;
}

ConnectionState CConnmanConnection::ParseConnectionState(const char *stateString)
{
  if (strcmp(stateString, "online") == 0)
    return NETWORK_CONNECTION_STATE_CONNECTED;
  else if (strcmp(stateString, "association") == 0)
    return NETWORK_CONNECTION_STATE_CONNECTING;
  else if (strcmp(stateString, "configuration") == 0)
    return NETWORK_CONNECTION_STATE_CONNECTING;
  else if (strcmp(stateString, "failure") == 0)
    return NETWORK_CONNECTION_STATE_FAILURE;
  else
  {
    // The state can be ready which means that the connection have been setup and is ready to be used.
    // Perhaps we want to differentiate this in GUI? Its not used but might be in case the active connection is lost.
    return NETWORK_CONNECTION_STATE_DISCONNECTED;
  }
}

void CConnmanConnection::UpdateConnection()
{
  m_name = m_properties["Name"].asString();

  m_state = ParseConnectionState(m_properties["State"].asString().c_str());

  if (strcmp(m_properties["Type"].asString().c_str(), "ethernet") == 0)
    m_type = NETWORK_CONNECTION_TYPE_WIRED;
  else if (strcmp(m_properties["Type"].asString().c_str(), "wifi") == 0)
    m_type = NETWORK_CONNECTION_TYPE_WIFI;
  else
    m_type = NETWORK_CONNECTION_TYPE_UNKNOWN;

  m_address = m_properties["IPv4"]["Address"].asString();
  m_netmask = m_properties["IPv4"]["Netmask"].asString();
  m_macaddress = m_properties["Ethernet"]["Address"].asString();
  m_gateway = m_properties["IPv4"]["Gateway"].asString();
  m_method  = m_properties["IPv4"]["Method"].asString();

  if (m_type == NETWORK_CONNECTION_TYPE_WIFI)
  {
    m_strength = m_properties["Strength"].asInteger();
    m_speed = m_properties["Ethernet"]["Speed"].asInteger();

    m_encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;
    if (strcmp(m_properties["PassphraseRequired"].asString().c_str(), "true") == 0)
    {
      if (m_properties["Security"].asString().empty())
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;
      else if (strcmp(m_properties["Security"].asString().c_str(), "none") == 0)
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;
      else if (strcmp(m_properties["Security"].asString().c_str(), "wep") == 0)
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_WEP;
      else if (strcmp(m_properties["Security"].asString().c_str(), "wpa") == 0)
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_WPA;
      else if (strcmp(m_properties["Security"].asString().c_str(), "psk") == 0)
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_WPA;
      else if (strcmp(m_properties["Security"].asString().c_str(), "rsn") == 0)
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_WPA;
      else if (strcmp(m_properties["Security"].asString().c_str(), "ieee8021x") == 0)
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_IEEE8021x;
      else
      {
        CLog::Log(LOGWARNING, "Connman: unknown connection encryption %s", m_properties["Security"].asString().c_str());
        m_encryption = NETWORK_CONNECTION_ENCRYPTION_UNKNOWN;
      }
    }
  }
  else
  {
    m_strength = 100;
    m_speed = m_properties["Ethernet"]["Speed"].asInteger();
    m_encryption = NETWORK_CONNECTION_ENCRYPTION_NONE;
  }
}
#endif

/*
 *      Copyright (C) 2009 Azur Digital Networks
 *      http://www.azurdigitalnetworks.net
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

#include "system.h"

#ifdef HAS_DBUS_SERVER
#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE 1
#endif
#include <dbus/dbus.h>
#include "DbusServer.h"
#include "threads/CriticalSection.h"
#include "Application.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include <map>
#include <queue>

using namespace DBUSSERVER;
using namespace std;

/* XML data to answer org.freedesktop.DBus.Introspectable.Introspect requests */
const char* psz_introspection_xml_data_root =
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node>\n"
"  <node name=\"Player\"/>\n"
"  <node name=\"TrackList\"/>\n"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"org.freedesktop.MediaPlayer\">\n"
"    <method name=\"Identity\">\n"
"      <arg type=\"s\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"MprisVersion\">\n"
"      <arg type=\"(qq)\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"Quit\">\n"
"    </method>\n"
"  </interface>\n"
"</node>\n"
;

const char* psz_introspection_xml_data_player =
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node>"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"org.freedesktop.MediaPlayer\">\n"
"    <method name=\"GetStatus\">\n"
"      <arg type=\"(iiii)\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"Prev\">\n"
"    </method>\n"
"    <method name=\"Next\">\n"
"    </method>\n"
"    <method name=\"Stop\">\n"
"    </method>\n"
"    <method name=\"Play\">\n"
"    </method>\n"
"    <method name=\"Pause\">\n"
"    </method>\n"
"    <method name=\"Repeat\">\n"
"      <arg type=\"b\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"VolumeSet\">\n"
"      <arg type=\"i\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"VolumeGet\">\n"
"      <arg type=\"i\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"PositionSet\">\n"
"      <arg type=\"i\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"PositionGet\">\n"
"      <arg type=\"i\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"GetMetadata\">\n"
"      <arg type=\"a{sv}\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"GetCaps\">\n"
"      <arg type=\"i\" direction=\"out\" />\n"
"    </method>\n"
"    <signal name=\"TrackChange\">\n"
"      <arg type=\"a{sv}\"/>\n"
"    </signal>\n"
"    <signal name=\"StatusChange\">\n"
"      <arg type=\"(iiii)\"/>\n"
"    </signal>\n"
"    <signal name=\"CapsChange\">\n"
"      <arg type=\"i\"/>\n"
"    </signal>\n"
"  </interface>\n"
"</node>\n"
;

const char* psz_introspection_xml_data_tracklist =
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node>"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"org.freedesktop.MediaPlayer\">\n"
"    <method name=\"AddTrack\">\n"
"      <arg type=\"s\" direction=\"in\" />\n"
"      <arg type=\"b\" direction=\"in\" />\n"
"      <arg type=\"i\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"DelTrack\">\n"
"      <arg type=\"i\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"GetMetadata\">\n"
"      <arg type=\"i\" direction=\"in\" />\n"
"      <arg type=\"a{sv}\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"GetCurrentTrack\">\n"
"      <arg type=\"i\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"GetLength\">\n"
"      <arg type=\"i\" direction=\"out\" />\n"
"    </method>\n"
"    <method name=\"SetLoop\">\n"
"      <arg type=\"b\" direction=\"in\" />\n"
"    </method>\n"
"    <method name=\"SetRandom\">\n"
"      <arg type=\"b\" direction=\"in\" />\n"
"    </method>\n"
"    <signal name=\"TrackListChange\">\n"
"      <arg type=\"i\" />\n"
"    </signal>\n"
"  </interface>\n"
"</node>\n"
;

DBusHandlerResult xbmc_dbus_message_handler_root(DBusConnection *connection, DBusMessage *message, void *user_data)
{
  return static_cast<CDbusServer *>(user_data)->got_message_root(connection,message);
}

DBusHandlerResult xbmc_dbus_message_handler_player(DBusConnection *connection, DBusMessage *message, void *user_data)
{
  return static_cast<CDbusServer *>(user_data)->got_message_player(connection,message);
}

DBusHandlerResult xbmc_dbus_message_handler_tracklist(DBusConnection *connection, DBusMessage *message, void *user_data)
{
  return static_cast<CDbusServer *>(user_data)->got_message_tracklist(connection,message);
}

/************************************************************************/
/* CDbusServer                                                         */
/************************************************************************/
CDbusServer* CDbusServer::m_pInstance = NULL;
CDbusServer::CDbusServer()
{
  m_bStop            = false;
  m_pThread          = NULL;
  m_bRunning         = false;
  p_conn             = NULL;
  p_application      = NULL;
}

void CDbusServer::RemoveInstance()
{
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance=NULL;
  }
}

CDbusServer* CDbusServer::GetInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = new CDbusServer();
  }
  return m_pInstance;
}

bool CDbusServer::connect(DBusConnection **conn)
{
    /* initialisation of the dbus connection */
  DBusError       error;
  dbus_error_init( &error );

  /* connect to the session bus */
  *conn = dbus_bus_get_private( DBUS_BUS_SESSION, &error );
  if( !*conn ) {
    CLog::Log(LOGERROR, " DS: Failed to connect to the D-Bus session daemon: %s", error.message );
    dbus_error_free( &error );
    return false;
  }

  dbus_connection_set_exit_on_disconnect(*conn, false);
  /* register a well-known name on the bus */
  dbus_bus_request_name( *conn, XBMC_MPRIS_DBUS_SERVICE, 0, &error );
  if( dbus_error_is_set( &error ) )
   {
       CLog::Log(LOGERROR, " DS: Error requesting service : %s", error.message );
       dbus_error_free( &error );
       return false;
   }
  dbus_error_free( &error );
  return true;
}

void CDbusServer::StartServer(CApplication *parent)
{
  CSingleLock lock(m_critSection);
  if (m_pThread)
    return;

  if (!connect(&p_conn)) {
    return;
  }

  p_application = parent;


  // ******* redirect to what path ? ( / == root, /Player == player, /Tracklist = tracklist )
  DBusObjectPathVTable xbmc_dbus_vtable_root = {NULL, xbmc_dbus_message_handler_root, NULL};
  if (!dbus_connection_register_object_path(p_conn, MPRIS_DBUS_ROOT_PATH, &xbmc_dbus_vtable_root, this ))
  {
    CLog::Log(LOGERROR, " DS: Not Enough memory for register dbus root object path." );
    return;
  }

  DBusObjectPathVTable xbmc_dbus_vtable_player = {NULL, xbmc_dbus_message_handler_player, NULL};
  if (!dbus_connection_register_object_path(p_conn, MPRIS_DBUS_PLAYER_PATH, &xbmc_dbus_vtable_player, this ))
  {
    CLog::Log(LOGERROR, " DS: Not Enough memory for register dbus player object path." );
    return;
  }

  DBusObjectPathVTable xbmc_dbus_vtable_tracklist = {NULL, xbmc_dbus_message_handler_tracklist, NULL};
  if (!dbus_connection_register_object_path(p_conn, MPRIS_DBUS_TRACKLIST_PATH, &xbmc_dbus_vtable_tracklist, this ))
  {
    CLog::Log(LOGERROR, " DS: Not Enough memory for register dbus tracklist object path." );
    return;
  }

  m_bStop = false;
  m_pThread = new CThread(this);
  m_pThread->Create();
  m_pThread->SetName("DbusServer");
}

void CDbusServer::StopServer(bool bWait)
{
  m_bStop = true;
  if (m_pThread && bWait)
  {
    m_pThread->WaitForThreadExit(2000);
    dbus_connection_close(p_conn);
    dbus_connection_unref( p_conn );
    delete m_pThread;
    m_pThread = NULL;
  }
}

void CDbusServer::Run()
{
  CLog::Log(LOGNOTICE, "DS: Starting DBUS server in Run Application aka thread");
  m_bRunning = false;

  while (!m_bStop)
  {
    // start listening until we timeout
    dbus_connection_read_write_dispatch(p_conn,1000);
  }

  CLog::Log(LOGNOTICE, "DS: DBUS server stopped");
  m_bRunning = false;
}

// ************ root message parser *********************
DBusHandlerResult CDbusServer::got_message_root( DBusConnection *connection, DBusMessage *message )
{
  if ((!connection) || (!message)) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  std::string method = dbus_message_get_member(message);
  if (method == "Introspect") {
    return handle_introspect_root( connection, message );
  } else if(method == "Identity") {
    return root_Identity( connection, message );
  } else if(method == "MprisVersion") {
    return root_MprisVersion( connection, message );
  }
  CLog::Log(LOGERROR, "DS: messages for root not yet used" );
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
// ******************* root distribution methods ***********************************
// ******************* INTROSPECTION
DBusHandlerResult CDbusServer::handle_introspect_root( DBusConnection *connection, DBusMessage *message )
{
  REPLY_INIT;
  OUT_ARGUMENTS;
  ADD_STRING( &psz_introspection_xml_data_root );
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::root_Identity( DBusConnection *connection, DBusMessage *message )
{
  REPLY_INIT;
  OUT_ARGUMENTS;
  char *psz_identity;
  if( asprintf( &psz_identity, "%s %s", XBMC_MPRIS_PACKAGE, XBMC_MPRIS_VERSION ) != -1 )
  {
    ADD_STRING( &psz_identity );
    free( psz_identity );
  }
  else
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::root_MprisVersion( DBusConnection *connection, DBusMessage *message )
{
 /*implemented version of the mpris spec */
  REPLY_INIT;
  OUT_ARGUMENTS;
  dbus_uint16_t i_major = XBMC_MPRIS_VERSION_MAJOR;
  dbus_uint16_t i_minor = XBMC_MPRIS_VERSION_MINOR;
  DBusMessageIter version;

  if( !dbus_message_iter_open_container( &args, DBUS_TYPE_STRUCT, NULL,&version ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  if( !dbus_message_iter_append_basic( &version, DBUS_TYPE_UINT16,&i_major ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  if( !dbus_message_iter_append_basic( &version, DBUS_TYPE_UINT16,&i_minor ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  if( !dbus_message_iter_close_container( &args, &version ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  REPLY_SEND;
}

// ************ player message parser *********************
DBusHandlerResult CDbusServer::got_message_player( DBusConnection *connection, DBusMessage *message)
{
  if ((!connection) || (!message)) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  std::string method = dbus_message_get_member(message);
  if (method == "Introspect") {
    return handle_introspect_player( connection, message );
  } else if (method == "GetStatus") {
    return player_GetStatus( connection, message );
  } else if (method == "Prev") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "Next") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "Stop") {
    return player_Stop( connection, message );
  } else if (method == "Play") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "Pause") {
    return player_Pause( connection, message );
  } else if (method == "Repeat") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "VolumeSet") {
    return player_VolumeSet( connection, message );
  } else if (method == "VolumeGet") {
    return player_VolumeGet( connection, message );
  } else if (method == "PositionGet") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "PositionSet") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "GetMetadata") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "GetCaps") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "TrackChange") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "StatusChange") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "CapsChange") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  CLog::Log(LOGERROR, "DS: messages for player not yet used");
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
// ******************* player distribution methods ***********************************
// ************** INTROSPECTION ****************
DBusHandlerResult CDbusServer::handle_introspect_player( DBusConnection *connection, DBusMessage *message )
{
  REPLY_INIT;
  OUT_ARGUMENTS;
  ADD_STRING( &psz_introspection_xml_data_player );
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::player_VolumeGet( DBusConnection *connection, DBusMessage *message )
{
  REPLY_INIT;
  OUT_ARGUMENTS;
  dbus_int32_t i_dbus_vol = lrint(p_application->GetVolume());
  ADD_INT32( &i_dbus_vol );
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::player_VolumeSet( DBusConnection *connection, DBusMessage *message )
{
/* set volume in percentage */
  REPLY_INIT;

  DBusError error;
  dbus_error_init( &error );

  dbus_int32_t i_dbus_vol;

  dbus_message_get_args( message, &error,
    DBUS_TYPE_INT32, &i_dbus_vol,
    DBUS_TYPE_INVALID );

  if( dbus_error_is_set( &error ) )
  {
    CLog::Log(LOGERROR, "DS: messages from dbus %s", error.message );
    dbus_error_free( &error );
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  p_application->SetVolume(i_dbus_vol);
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::player_GetStatus( DBusConnection *connection, DBusMessage *message )
{
/* returns the current status as a struct of 4 ints */
/*
  First   0 = Playing, 1 = Paused, 2 = Stopped.
  Second  0 = Playing linearly , 1 = Playing randomly.
  Third   0 = Go to the next element once the current has finished playing , 1 = Repeat the current element
  Fourth  0 = Stop playing once the last element has been played, 1 = Never give up playing *
*/
  REPLY_INIT;
  OUT_ARGUMENTS;

  dbus_uint16_t i_status_1 = 2;

  if(p_application->IsPaused()) {
    if (p_application->IsPlaying()) {
      i_status_1 = 1;
    }
  } else {
    if (p_application->IsPlaying()) {
      i_status_1 = 0;
    }
  }

  dbus_uint16_t i_status_2 = 0;
  dbus_uint16_t i_status_3 = 0;
  dbus_uint16_t i_status_4 = 0;

  DBusMessageIter status;

  if( !dbus_message_iter_open_container( &args, DBUS_TYPE_STRUCT, NULL, &status ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  if( !dbus_message_iter_append_basic( &status, DBUS_TYPE_UINT16, &i_status_1 ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  if( !dbus_message_iter_append_basic( &status, DBUS_TYPE_UINT16, &i_status_2 ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  if( !dbus_message_iter_append_basic( &status, DBUS_TYPE_UINT16, &i_status_3 ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  if( !dbus_message_iter_append_basic( &status, DBUS_TYPE_UINT16, &i_status_4 ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  if( !dbus_message_iter_close_container( &args, &status ) )
    return DBUS_HANDLER_RESULT_NEED_MEMORY;
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::player_Stop( DBusConnection *connection, DBusMessage *message )
{
  REPLY_INIT;
  OUT_ARGUMENTS;
  p_application->StopPlaying();
  REPLY_SEND;
}

DBusHandlerResult CDbusServer::player_Pause( DBusConnection *connection, DBusMessage *message )
{
/*
 * TODO a real pause system not stop ;)
 */

  REPLY_INIT;
  OUT_ARGUMENTS;
  p_application->StopPlaying();
  REPLY_SEND;
}



// ************ tracklist message parser *********************
DBusHandlerResult CDbusServer::got_message_tracklist( DBusConnection *connection, DBusMessage *message )
{
  if ((!connection) || (!message)) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  std::string method = dbus_message_get_member(message);
  if (method == "Introspect") {
    return handle_introspect_tracklist( connection, message );
  } else if (method == "AddTrack") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "DelTrack") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "GetMetadata") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "GetCurrentTrack") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "GetLength") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "SetLoop") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "SetRandom") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } else if (method == "TrackListChange") {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  CLog::Log(LOGERROR, "DS: messages for tracklist not yet used");
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

// ******************* tracklist distribution methods ***********************************
// ************** INTROSPECTION ****************
DBusHandlerResult CDbusServer::handle_introspect_tracklist( DBusConnection *connection, DBusMessage *message )
{
  REPLY_INIT;
  OUT_ARGUMENTS;
  ADD_STRING( &psz_introspection_xml_data_tracklist );
  REPLY_SEND;
}

#endif // HAS_DBUS_SERVER

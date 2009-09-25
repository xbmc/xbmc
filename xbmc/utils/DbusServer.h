#ifndef __DBUS_SERVER_H__
#define __DBUS_SERVER_H__

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
#include "Thread.h"
#include "CriticalSection.h"
#include <map>
#include <queue>
#include <dbus/dbus.h>

#ifdef HAS_DBUS_SERVER

/* MPRIS VERSION */
#define XBMC_MPRIS_VERSION_MAJOR     1
#define XBMC_MPRIS_VERSION_MINOR     0

/* PACKAGE/VERSION */
#define XBMC_MPRIS_VERSION     "0.9.04"
#define XBMC_MPRIS_PACKAGE     "xbmc"

/* DBUS IDENTIFIERS */

/* name registered on the session bus */
#define XBMC_MPRIS_DBUS_SERVICE     "org.mpris.xbmc"
#define XBMC_MPRIS_DBUS_INTROSPECT  "Introspect"
#define MPRIS_DBUS_INTERFACE        "org.freedesktop.MediaPlayer"
#define MPRIS_DBUS_ROOT_PATH        "/"
#define MPRIS_DBUS_PLAYER_PATH      "/Player"
#define MPRIS_DBUS_TRACKLIST_PATH   "/TrackList"

/* MACROS */

#define DBUS_METHOD( method_function ) \
    DBusHandlerResult method_function( DBusConnection *p_conn, DBusMessage *p_from, void *p_this )

#define DBUS_METHOD_IMPL( method_function ) \
    DBusHandlerResult CDbusServer::method_function( DBusConnection *p_conn, DBusMessage *p_from, void *p_this )

#define DBUS_SIGNAL( signal_function ) \
    DBusHandlerResult signal_function( DBusConnection *p_conn, void *p_data )

#define REPLY_INIT \
    DBusMessage* p_msg = dbus_message_new_method_return( message ); \
    if( !p_msg ) return DBUS_HANDLER_RESULT_NEED_MEMORY; \

#define REPLY_SEND \
    if( !dbus_connection_send( p_conn, p_msg, NULL ) ) \
        return DBUS_HANDLER_RESULT_NEED_MEMORY; \
    dbus_connection_flush( p_conn ); \
    dbus_message_unref( p_msg ); \
    return DBUS_HANDLER_RESULT_HANDLED

#define SIGNAL_INIT( signal ) \
    DBusMessage *p_msg = dbus_message_new_signal( MPRIS_DBUS_PLAYER_PATH, \
        MPRIS_DBUS_INTERFACE, signal ); \
    if( !p_msg ) return DBUS_HANDLER_RESULT_NEED_MEMORY; \

#define SIGNAL_SEND \
    if( !dbus_connection_send( p_conn, p_msg, NULL ) ) \
        return DBUS_HANDLER_RESULT_NEED_MEMORY; \
    dbus_message_unref( p_msg ); \
    dbus_connection_flush( p_conn ); \
    return DBUS_HANDLER_RESULT_HANDLED

#define OUT_ARGUMENTS \
    DBusMessageIter args; \
    dbus_message_iter_init_append( p_msg, &args )

#define DBUS_ADD( dbus_type, value ) \
    if( !dbus_message_iter_append_basic( &args, dbus_type, value ) ) \
        return DBUS_HANDLER_RESULT_NEED_MEMORY

#define ADD_STRING( s ) DBUS_ADD( DBUS_TYPE_STRING, s )
#define ADD_BOOL( b ) DBUS_ADD( DBUS_TYPE_BOOLEAN, b )
#define ADD_INT32( i ) DBUS_ADD( DBUS_TYPE_INT32, i )
#define ADD_BYTE( b ) DBUS_ADD( DBUS_TYPE_BYTE, b )


/* Handle  messages reception */
#define METHOD_FUNC( method, function ) \
    else if( dbus_message_is_method_call( p_from, MPRIS_DBUS_INTERFACE, method ) )\
        return function( p_conn, p_from, p_this )

#endif

class CApplication;

namespace DBUSSERVER
{

  /**********************************************************************/
  /* DBUSServer Class                                                   */
  /**********************************************************************/
  class CDbusServer : public IRunnable
  {
  public:
    static void RemoveInstance();
    static CDbusServer* GetInstance();
    virtual ~CDbusServer() {}

    // IRunnable entry point for thread
    virtual void  Run();

    bool Running()
    {
      return m_bRunning;
    }

    // start / stop server
    void StartServer(CApplication* );
    void StopServer(bool bWait);

    // get events
    bool ExecuteNextAction();

    friend DBusHandlerResult xbmc_dbus_message_handler_root(DBusConnection *, DBusMessage *, void *);
    DBusHandlerResult got_message_root(DBusConnection *, DBusMessage *);

    friend DBusHandlerResult xbmc_dbus_message_handler_player(DBusConnection *, DBusMessage *, void *);
    DBusHandlerResult got_message_player(DBusConnection *, DBusMessage *);

    friend DBusHandlerResult xbmc_dbus_message_handler_tracklist(DBusConnection *, DBusMessage *, void *);
    DBusHandlerResult got_message_tracklist(DBusConnection *, DBusMessage *);

  protected:
    CDbusServer();
    void ProcessEvents();
    CThread*         m_pThread;
    static CDbusServer* m_pInstance;
    bool             m_bStop;
    bool             m_bRunning;
    CCriticalSection m_critSection;
    CApplication     *p_application;

  private:
    DBusHandlerResult handle_introspect_root( DBusConnection *, DBusMessage * );
    DBusHandlerResult handle_introspect_player( DBusConnection *, DBusMessage * );
    DBusHandlerResult handle_introspect_tracklist( DBusConnection *, DBusMessage * );
    DBusHandlerResult root_Identity( DBusConnection *, DBusMessage * );
    DBusHandlerResult root_MprisVersion( DBusConnection *, DBusMessage * );
    DBusHandlerResult player_VolumeSet( DBusConnection *, DBusMessage * );
    DBusHandlerResult player_VolumeGet( DBusConnection *, DBusMessage * );
    DBusHandlerResult player_GetStatus( DBusConnection *, DBusMessage * );
    DBusHandlerResult player_Stop( DBusConnection *, DBusMessage * );
    DBusHandlerResult player_Pause( DBusConnection *, DBusMessage * );

    bool connect(DBusConnection **);
    DBusConnection *p_conn;
    bool _ok;

  };

}

#endif // __DBUS_SERVER_H__

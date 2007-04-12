///////////////////////////////////////////////////////////////////////////////
//
//  Module: MailMsg.h
//
//    Desc: This class encapsulates the MAPI and CMC mail functions.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAILMSG_H_
#define _MAILMSG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <xcmc.h>          // CMC function defs
#include <mapi.h>          // MAPI function defs

#ifndef TStrStrMap
#include <map>

typedef std::map<CStdString, CStdString> TStrStrMap;
#endif // !defined TStrStrMap

//
// Define CMC entry points
//
typedef CMC_return_code (FAR PASCAL *LPCMCLOGON) \
   (CMC_string, CMC_string, CMC_string, CMC_object_identifier, \
   CMC_ui_id, CMC_uint16, CMC_flags, CMC_session_id FAR*, \
   CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCSEND) \
   (CMC_session_id, CMC_message FAR*, CMC_flags, \
   CMC_ui_id, CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCLOGOFF) \
   (CMC_session_id, CMC_ui_id, CMC_flags, CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCQUERY) \
   (CMC_session_id, CMC_enum, CMC_buffer, CMC_extension FAR*);


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CMailMsg
// 
// See the module comment at top of file.
//
class CMailMsg  
{
public:
	CMailMsg();
	virtual ~CMailMsg();

   //-----------------------------------------------------------------------------
   // SetTo
   //    Sets the Email:To address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Only one To address can be set.  If called more than once
   //    the last address will be used.
   //
   CMailMsg& 
   SetTo(
      CStdString sAddress, 
      CStdString sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetCc
   //    Sets the Email:Cc address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Multiple Cc addresses can be set.
   //
   CMailMsg& 
   SetCc(
      CStdString sAddress, 
      CStdString sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetBc
   //    Sets the Email:Bcc address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Multiple Bcc addresses can be set.
   //
   CMailMsg& 
   SetBc(
      CStdString sAddress, 
      CStdString sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetFrom
   //    Sets the Email:From address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Only one From address can be set.  If called more than once
   //    the last address will be used.
   //
   CMailMsg& 
   SetFrom(
      CStdString sAddress, 
      CStdString sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetSubect
   //    Sets the Email:Subject
   //
   // Parameters
   //    sSubject    Subject
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    none
   //
   CMailMsg& 
   SetSubject(
      CStdString sSubject
      ) {m_sSubject = sSubject; return *this;};

   //-----------------------------------------------------------------------------
   // SetMessage
   //    Sets the Email message body
   //
   // Parameters
   //    sMessage    Message body
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    none
   //
   CMailMsg& 
   SetMessage(
      CStdString sMessage
      ) {m_sMessage = sMessage; return *this;};

   //-----------------------------------------------------------------------------
   // AddAttachment
   //    Attaches a file to the email
   //
   // Parameters
   //    sAttachment Fully qualified file name
   //    sTitle      File display name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    none
   //
   CMailMsg& 
   AddAttachment(
      CStdString sAttachment, 
      CStdString sTitle = _T("")
      );

   //-----------------------------------------------------------------------------
   // Send
   //    Send the email.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if succesful
   //
   // Remarks
   //    First simple MAPI is used if unsucessful CMC is used.
   //
   BOOL Send();

protected:

   //-----------------------------------------------------------------------------
   // CMCSend
   //    Send email using CMC functions.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if successful
   //
   // Remarks
   //    none
   //
   BOOL CMCSend();

   //-----------------------------------------------------------------------------
   // MAPISend
   //    Send email using MAPI functions.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if successful
   //
   // Remarks
   //    none
   //
   BOOL MAPISend();

   //-----------------------------------------------------------------------------
   // Initialize
   //    Initialize MAPI32.dll
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if successful
   //
   // Remarks
   //    none
   //
   BOOL Initialize();

   //-----------------------------------------------------------------------------
   // Uninitialize
   //    Uninitialize MAPI32.dll
   //
   // Parameters
   //    none
   //
   // Return Values
   //    void
   //
   // Remarks
   //    none
   //
   void Uninitialize();

   TStrStrMap     m_from;                       // From <address,name>
   TStrStrMap     m_to;                         // To <address,name>
   TStrStrMap     m_cc;                         // Cc <address,name>
   TStrStrMap     m_bcc;                        // Bcc <address,name>
   TStrStrMap     m_attachments;                // Attachment <file,title>
   CStdString     m_sSubject;                   // EMail subject
   CStdString     m_sMessage;                   // EMail message

   HMODULE        m_hMapi;                      // Handle to MAPI32.DLL
   LPCMCQUERY     m_lpCmcQueryConfiguration;    // Cmc func pointer
   LPCMCLOGON     m_lpCmcLogon;                 // Cmc func pointer
   LPCMCSEND		m_lpCmcSend;                  // Cmc func pointer
   LPCMCLOGOFF		m_lpCmcLogoff;                // Cmc func pointer
   LPMAPILOGON		m_lpMapiLogon;                // Mapi func pointer
   LPMAPISENDMAIL m_lpMapiSendMail;             // Mapi func pointer
   LPMAPILOGOFF   m_lpMapiLogoff;               // Mapi func pointer
   
   BOOL           m_bReady;                     // MAPI is loaded
};

#endif	// #ifndef _MAILMSG_H_

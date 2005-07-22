///////////////////////////////////////////////////////////////////////////////
//
//  Module: MailMsg.cpp
//
//    Desc: See MailMsg.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MailMsg.h"

CMailMsg::CMailMsg()
{
	m_sSubject        = _T("");
	m_sMessage        = _T("");
	m_lpCmcLogon      = NULL;
	m_lpCmcSend       = NULL;
	m_lpCmcLogoff     = NULL;
	m_lpMapiLogon     = NULL;
	m_lpMapiSendMail  = NULL;
	m_lpMapiLogoff    = NULL;
	m_bReady          = FALSE;
}

CMailMsg::~CMailMsg()
{
	if (m_bReady)
		Uninitialize();
}

CMailMsg& CMailMsg::SetFrom(CStdString sAddress, CStdString sName)
{
	if (m_bReady || Initialize())
	{
		// only one sender allowed
		if (m_from.size())
			m_from.empty();

		m_from[sAddress] = sName;
	}

	return *this;
}

CMailMsg& CMailMsg::SetTo(CStdString sAddress, CStdString sName)
{
   if (m_bReady || Initialize())
   {
      // only one recipient allowed
      if (m_to.size())
         m_to.empty();

      m_to[sAddress] = sName;
   }

   return *this;
}

CMailMsg& CMailMsg::SetCc(CStdString sAddress, CStdString sName)
{
   if (m_bReady || Initialize())
   {
      m_cc[sAddress] = sName;
   }

   return *this;
}

CMailMsg& CMailMsg::SetBc(CStdString sAddress, CStdString sName)
{
   if (m_bReady || Initialize())
   {
      m_bcc[sAddress] = sName;
   }

   return *this;
}

CMailMsg& CMailMsg::AddAttachment(CStdString sAttachment, CStdString sTitle)
{
   if (m_bReady || Initialize())
   {
      m_attachments[sAttachment] = sTitle;
   }

   return *this;
}

BOOL CMailMsg::Send()
{
   // try mapi
   if (MAPISend())
      return TRUE;

   // try cmc
   if (CMCSend())
      return TRUE;

   return FALSE;
}

BOOL CMailMsg::MAPISend()
{
	TStrStrMap::iterator p;
	int                  nIndex = 0;
	int                  nRecipients = 0;
	MapiRecipDesc*       pRecipients = NULL;
	int                  nAttachments = 0;
	MapiFileDesc*        pAttachments = NULL;
	ULONG                status = 0;
	MapiMessage          message;

	USES_CONVERSION;

	if (m_bReady || Initialize())
	{
		nRecipients = m_to.size() + m_cc.size() + m_bcc.size() + m_from.size();
		if (nRecipients)
			pRecipients = new MapiRecipDesc[nRecipients];

		nAttachments = m_attachments.size();
		if (nAttachments)
			pAttachments = new MapiFileDesc[nAttachments];

		if (pRecipients)
		{
			if (m_from.size())
			{
				// set from
				pRecipients[nIndex].ulReserved                 = 0;
				pRecipients[nIndex].ulRecipClass               = MAPI_ORIG;
				pRecipients[nIndex].lpszAddress                = T2A((LPTSTR)(LPCTSTR)m_from.begin()->first);
				pRecipients[nIndex].lpszName                   = T2A((LPTSTR)(LPCTSTR)m_from.begin()->second);
				pRecipients[nIndex].ulEIDSize                  = 0;
				pRecipients[nIndex].lpEntryID                  = NULL;
				nIndex++;
			}

			if (m_to.size())
			{
				// set to
				pRecipients[nIndex].ulReserved                 = 0;
				pRecipients[nIndex].ulRecipClass               = MAPI_TO;
				pRecipients[nIndex].lpszAddress                = T2A((LPTSTR)(LPCTSTR)m_to.begin()->first);
				pRecipients[nIndex].lpszName                   = T2A((LPTSTR)(LPCTSTR)m_to.begin()->second);
				pRecipients[nIndex].ulEIDSize                  = 0;
				pRecipients[nIndex].lpEntryID                  = NULL;
				nIndex++;
			}

			if (m_cc.size())
			{
				// set cc's
				for (p = m_cc.begin(); p != m_cc.end(); p++, nIndex++)
				{
					pRecipients[nIndex].ulReserved         = 0;
					pRecipients[nIndex].ulRecipClass       = MAPI_CC;
					pRecipients[nIndex].lpszAddress        = T2A((LPTSTR)(LPCTSTR)p->first);
					pRecipients[nIndex].lpszName           = T2A((LPTSTR)(LPCTSTR)p->second);
					pRecipients[nIndex].ulEIDSize          = 0;
					pRecipients[nIndex].lpEntryID          = NULL;
				}
			}

			if (m_bcc.size())
			{
				// set bcc
				for (p = m_bcc.begin(); p != m_bcc.end(); p++, nIndex++)
				{
					pRecipients[nIndex].ulReserved         = 0;
					pRecipients[nIndex].ulRecipClass       = MAPI_BCC;
					pRecipients[nIndex].lpszAddress        = T2A((LPTSTR)(LPCTSTR)p->first);
					pRecipients[nIndex].lpszName           = T2A((LPTSTR)(LPCTSTR)p->second);
					pRecipients[nIndex].ulEIDSize          = 0;
					pRecipients[nIndex].lpEntryID          = NULL;
				}
			}
		}

		if (pAttachments)
		{
			// add attachments
			for (p = m_attachments.begin(), nIndex = 0;
			p != m_attachments.end(); p++, nIndex++)
			{
				pAttachments[nIndex].ulReserved        = 0;
				pAttachments[nIndex].flFlags           = 0;
				pAttachments[nIndex].nPosition         = 0xFFFFFFFF;
				pAttachments[nIndex].lpszPathName      = T2A((LPTSTR)(LPCTSTR)p->first);
				pAttachments[nIndex].lpszFileName      = T2A((LPTSTR)(LPCTSTR)p->second);
				pAttachments[nIndex].lpFileType        = NULL;
			}
		}

		message.ulReserved                        = 0;
		message.lpszSubject                       = T2A((LPTSTR)(LPCTSTR)m_sSubject);
		message.lpszNoteText                      = T2A((LPTSTR)(LPCTSTR)m_sMessage);
		message.lpszMessageType                   = NULL;
		message.lpszDateReceived                  = NULL;
		message.lpszConversationID                = NULL;
		message.flFlags                           = 0;
		message.lpOriginator                      = m_from.size() ? pRecipients : NULL;
		message.nRecipCount                       = nRecipients - m_from.size(); // don't count originator
		message.lpRecips                          = nRecipients - m_from.size() ? &pRecipients[m_from.size()] : NULL;
		message.nFileCount                        = nAttachments;
		message.lpFiles                           = nAttachments ? pAttachments : NULL;


		LHANDLE hMapiSession = NULL;
		m_lpMapiLogon(NULL, NULL, NULL, 0, 0, &hMapiSession);
		status = m_lpMapiSendMail(hMapiSession, 0, &message, MAPI_DIALOG, 0);
		m_lpMapiLogoff(hMapiSession, NULL, 0, 0);

		if (pRecipients)
			delete [] pRecipients;

		if (nAttachments)
			delete [] pAttachments;
	}

	return (SUCCESS_SUCCESS == status);
}

BOOL CMailMsg::CMCSend()
{
   TStrStrMap::iterator p;
   int                  nIndex = 0;
   CMC_recipient*       pRecipients;
   CMC_attachment*      pAttachments;
   CMC_session_id       session;
   CMC_return_code      status = 0;
   CMC_message          message;
   CMC_boolean          bAvailable = FALSE;
   CMC_time             t_now = {0};

   USES_CONVERSION;

   if (m_bReady || Initialize())
   {
      pRecipients = new CMC_recipient[m_to.size() + m_cc.size() + m_bcc.size() + m_from.size()];
      pAttachments = new CMC_attachment[m_attachments.size()];

      // set cc's
      for (p = m_cc.begin(); p != m_cc.end(); p++, nIndex++)
      {
         pRecipients[nIndex].name                = T2A((LPTSTR)(LPCTSTR)p->second);
         pRecipients[nIndex].name_type           = CMC_TYPE_INDIVIDUAL;
         pRecipients[nIndex].address             = T2A((LPTSTR)(LPCTSTR)p->first);
         pRecipients[nIndex].role                = CMC_ROLE_CC;
         pRecipients[nIndex].recip_flags         = 0;
         pRecipients[nIndex].recip_extensions    = NULL;
      }

      // set bcc
      for (p = m_bcc.begin(); p != m_bcc.end(); p++, nIndex++)
      {
         pRecipients[nIndex].name                = T2A((LPTSTR)(LPCTSTR)p->second);
         pRecipients[nIndex].name_type           = CMC_TYPE_INDIVIDUAL;
         pRecipients[nIndex].address             = T2A((LPTSTR)(LPCTSTR)p->first);
         pRecipients[nIndex].role                = CMC_ROLE_BCC;
         pRecipients[nIndex].recip_flags         = 0;
         pRecipients[nIndex].recip_extensions    = NULL;
      }

      // set to
      pRecipients[nIndex].name                   = T2A((LPTSTR)(LPCTSTR)m_to.begin()->second);
      pRecipients[nIndex].name_type              = CMC_TYPE_INDIVIDUAL;
      pRecipients[nIndex].address                = T2A((LPTSTR)(LPCTSTR)m_to.begin()->first);
      pRecipients[nIndex].role                   = CMC_ROLE_TO;
      pRecipients[nIndex].recip_flags            = 0;
      pRecipients[nIndex].recip_extensions       = NULL;

      // set from
      pRecipients[nIndex+1].name                 = T2A((LPTSTR)(LPCTSTR)m_from.begin()->second);
      pRecipients[nIndex+1].name_type            = CMC_TYPE_INDIVIDUAL;
      pRecipients[nIndex+1].address              = T2A((LPTSTR)(LPCTSTR)m_from.begin()->first);
      pRecipients[nIndex+1].role                 = CMC_ROLE_ORIGINATOR;
      pRecipients[nIndex+1].recip_flags          = CMC_RECIP_LAST_ELEMENT;
      pRecipients[nIndex+1].recip_extensions     = NULL;

      // add attachments
      for (p = m_attachments.begin(), nIndex = 0;
           p != m_attachments.end(); p++, nIndex++)
      {
         pAttachments[nIndex].attach_title       = T2A((LPTSTR)(LPCTSTR)p->second);
         pAttachments[nIndex].attach_type        = NULL;
         pAttachments[nIndex].attach_filename    = T2A((LPTSTR)(LPCTSTR)p->first);
         pAttachments[nIndex].attach_flags       = 0;
         pAttachments[nIndex].attach_extensions  = NULL;
      }
      pAttachments[nIndex-1].attach_flags        = CMC_ATT_LAST_ELEMENT;

      message.message_reference                 = NULL;
      message.message_type                      = NULL;
      message.subject                           = T2A((LPTSTR)(LPCTSTR)m_sSubject);
      message.time_sent                         = t_now;
      message.text_note                         = T2A((LPTSTR)(LPCTSTR)m_sMessage);
      message.recipients                        = pRecipients;
      message.attachments                       = pAttachments;
      message.message_flags                     = 0;
      message.message_extensions                = NULL;

      status = m_lpCmcQueryConfiguration(
                  0,
                  CMC_CONFIG_UI_AVAIL,
                  (void*)&bAvailable,
                  NULL
                  );

      if (CMC_SUCCESS == status && bAvailable)
      {
         status = m_lpCmcLogon(
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     0,
                     CMC_VERSION,
                     CMC_LOGON_UI_ALLOWED |
                     CMC_ERROR_UI_ALLOWED,
                     &session,
                     NULL
                     );

         if (CMC_SUCCESS == status)
         {
            status = m_lpCmcSend(session, &message, 0, 0, NULL);

            m_lpCmcLogoff(session, NULL, CMC_LOGON_UI_ALLOWED, NULL);
         }
      }

      delete [] pRecipients;
      delete [] pAttachments;
   }

   return ((CMC_SUCCESS == status) && bAvailable);
}

BOOL CMailMsg::Initialize()
{
   m_hMapi = ::LoadLibrary(_T("mapi32.dll"));

   if (!m_hMapi)
      return FALSE;

   m_lpCmcQueryConfiguration = (LPCMCQUERY)::GetProcAddress(m_hMapi, "cmc_query_configuration");
   m_lpCmcLogon = (LPCMCLOGON)::GetProcAddress(m_hMapi, "cmc_logon");
   m_lpCmcSend = (LPCMCSEND)::GetProcAddress(m_hMapi, "cmc_send");
   m_lpCmcLogoff = (LPCMCLOGOFF)::GetProcAddress(m_hMapi, "cmc_logoff");
   m_lpMapiLogon = (LPMAPILOGON)::GetProcAddress(m_hMapi, "MAPILogon");
   m_lpMapiSendMail = (LPMAPISENDMAIL)::GetProcAddress(m_hMapi, "MAPISendMail");
   m_lpMapiLogoff = (LPMAPILOGOFF)::GetProcAddress(m_hMapi, "MAPILogoff");

   m_bReady = (m_lpCmcLogon && m_lpCmcSend && m_lpCmcLogoff) ||
              (m_lpMapiLogon && m_lpMapiSendMail && m_lpMapiLogoff);

   return m_bReady;
}

void CMailMsg::Uninitialize()
{
   ::FreeLibrary(m_hMapi);
}
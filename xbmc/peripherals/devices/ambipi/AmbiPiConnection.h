#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
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
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

#include "utils/AutoPtrHandle.h"

namespace PERIPHERALS
{

  class CAmbiPiConnection : private CThread
  {
  public:
    CAmbiPiConnection(void);
    ~CAmbiPiConnection(void);
    void Connect(const CStdString ip_address_or_name, unsigned int port);
    void Disconnect(void);
    void Send(const BYTE *buffer, int length);
    bool IsConnected(void) const;

  protected:
    void Process(void);

  private:
    struct addrinfo *GetAddressInfo(const CStdString ip_address_or_name, unsigned int port);

    void AttemptConnection(void);
    void AttemptConnection(struct addrinfo *pAddressInfo);

    AUTOPTR::CAutoPtrSocket           m_socket;
    CStdString                        m_ip_address_or_name;
    unsigned int                      m_port;

    bool                              m_bConnected;
    bool                              m_bConnecting;
    CCriticalSection                  m_critSection;
  };
}

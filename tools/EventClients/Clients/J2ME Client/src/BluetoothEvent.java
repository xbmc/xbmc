/*
* XBMC Media Center
* UDP Event Server
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
public interface BluetoothEvent {
  public final int RECV_SYSTEM  = 0;
  public final int RECV_COMMAND = 1;
  public final int RECV_IMAGE   = 2;
    
  // The header is a byte IRL. 0 - 255 and tells packet type
  public void Recv(int Header, byte[] Payload);
}

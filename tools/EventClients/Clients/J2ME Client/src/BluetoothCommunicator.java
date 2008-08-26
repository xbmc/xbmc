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
import javax.microedition.io.Connector;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import javax.microedition.io.StreamConnection;
import java.lang.NullPointerException;
import java.util.Vector;
import javax.bluetooth.*;
import javax.bluetooth.LocalDevice;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Display;
import java.lang.*;

public class BluetoothCommunicator extends Thread
{
  private boolean Run;
  private Form    Log;
  private BluetoothEvent recvEvent;

  protected static String Address = null;
  private static StreamConnection Connection = null;
  private static DataInputStream  inputStream = null;
  private static DataOutputStream outputStream = null;

  public BluetoothCommunicator(BluetoothEvent recvEvent, Form Log)
  {
    this.recvEvent = recvEvent;
    this.Log = Log;
  }

  public static boolean Connect(Form Log, String addr)
  {
    Log.append("Going to connect\n");
    try
    {
      Connection = (StreamConnection)Connector.open(addr);

      if (Connection == null)
      {
        Log.append("Connector.open returned null\n");
        return false;
      }
      else
      {
        Log.append("Connector.open returned connection\n");
        inputStream  = Connection.openDataInputStream();
        outputStream = Connection.openDataOutputStream();
        Address = addr;
        return true;
      }
    }
    catch (Exception e)
    {
      Log.append("Exception" + e.getMessage() + "\n");
      try
      {
        if (Connection != null)
          Connection.close();
      }
      catch (Exception ee)
      {
        Log.append("Exception " + ee.getMessage() + "\n");
      }
      Connection = null;
      return false;
    }
  }

  public static boolean Handshake(Form Log, Display disp)
  {
      if (Connection == null)
        return false;
      try
      {
        LocalDevice localDevice = LocalDevice.getLocalDevice();
        Send(localDevice.getFriendlyName());

        return true;
      }
      catch(NullPointerException NPE)
      {
        Log.append("Null " + NPE.getMessage());
      }
      catch (IndexOutOfBoundsException IOBE)
      {
        Log.append("IndexOutOfBounds " + IOBE.getMessage());
      }
      catch (IllegalArgumentException IAE)
      {
        Log.append("illegalArg " + IAE.getMessage());
      }
      catch(Exception e)
      {
        Log.append("Exception in handshake " + e.getMessage());
      }
      return false;
  }
  public void run()
  {
    Run = true;
    while (Run)
    {
      try
      {
        if (inputStream.available() <= 0)
            sleep(10);
        else
        {
          Log.append("run:Ava " + inputStream.available() + "\n");
          byte []b = new byte[1];
          b[0] = (byte)255; // safe thing for now
          inputStream.read(b, 0, 1);
          byte[] recv = null;
          switch (b[0])
          {
            case BluetoothEvent.RECV_SYSTEM:
              recv = new byte[]{ inputStream.readByte() };
              break;
            case BluetoothEvent.RECV_COMMAND:
              recv = new byte[]{ inputStream.readByte() };
              break;
            case BluetoothEvent.RECV_IMAGE:
              Log.append("Recv_Image\n");
              recv = RecvImage(Log);
              break;
          }
          if (recv != null)
            recvEvent.Recv(b[0], recv);
        }
      }
      catch (Exception e)
      {
        Log.append("RecvException " + e.getMessage());
      }
    }
  }
  public void close()
  {
    Run = false;
  }
  public static byte[] RecvImage(Form Log)
  {
    try
    {
      int length = inputStream.readInt();
      Log.append("readUnsignedShort: " + length + "\n");
      byte[] arrayToUse = new byte[length];
      inputStream.readFully(arrayToUse, 0, length);
      return arrayToUse;
    }
    catch (Exception e )
    {
      Log.append("Exception RecvImage " + e.getMessage());
      return null;
    }
  }

  public static int Recv(byte[] arrayToUse, Form Log )
  {
    if (Connection == null)
      return -1;
    try
    {
      return inputStream.read(arrayToUse);
    }
    catch (Exception e)
    {
      Log.append("Exception in Recv " + e.getMessage());
      return -1;
    }
  }

  public static boolean Send(byte[] packet)
  {
    if (Connection == null)
      return false;
    try
    {
      outputStream.write(packet, 0, packet.length);
      return true;
    }
    catch (Exception e)
    {
      return false;
    }
  }

  public static boolean Send(String packet)
  {
    return Send(packet.getBytes());
  }

  public static void Disconnect()
  {
    if (Connection == null)
      return;
    
    try
    {
      byte[] packet = { '0', '0' };
      Send(packet);
      Connection.close();
    }
    catch (Exception e)
    { }
    
    Connection = null;
  }
}

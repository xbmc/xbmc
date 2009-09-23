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

import java.util.Vector;
import javax.bluetooth.DeviceClass;
import javax.bluetooth.DiscoveryAgent;
import javax.bluetooth.DiscoveryListener;
import javax.bluetooth.LocalDevice;
import javax.bluetooth.RemoteDevice;
import javax.bluetooth.ServiceRecord;
import javax.bluetooth.UUID;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

public class MainScreen extends MIDlet implements CommandListener, KeyHandler, BluetoothEvent
{
  private boolean LogActive = true;
  private final int VersionMajor = 0;
  private final int VersionMinor = 1;
  private final String Version = (VersionMajor + "." + VersionMinor);

  private final Command CMD_OK            = new Command("OK", Command.OK, 1);
  private final Command CMD_CONNECT       = new Command("Connect", Command.OK, 1);
  private final Command CMD_DISCONNECT    = new Command("Disconnect", Command.CANCEL, 1);
  private final Command CMD_EXIT          = new Command("Exit", Command.CANCEL, 1);
  private final Command CMD_LOG           = new Command("Log", Command.OK, 1);
  private final Command CMD_SHOWREMOTE    = new Command("Remote", Command.OK, 1);
  private Display display;
  private Form MainForm;

  private KeyCanvas keyHandler;
  private BluetoothCommunicator RecvThread;

  private final int CONNECTED     = 1;
  private final int NOT_CONNECTED  = -1;
  private final int CONNECTING    = 0;

  private int Connect = NOT_CONNECTED;

  public MainScreen()
  {
    display = Display.getDisplay(this);
    keyHandler = new KeyCanvas(this);
    keyHandler.addCommand(CMD_LOG);
    keyHandler.addCommand(CMD_DISCONNECT);
    keyHandler.setCommandListener(this);
  }


  public void Disconnect()
  {
    RecvThread.close();
    BluetoothCommunicator.Disconnect();
    MainForm.removeCommand(CMD_DISCONNECT);
    MainForm.removeCommand(CMD_SHOWREMOTE);
    MainForm.addCommand(CMD_CONNECT);
    Connect = NOT_CONNECTED;
  }

  public void Connect()
  {
    if (Connect == CONNECTED)
      return;
    Connect = CONNECTING;
    MainForm = CreateForm("Connect log");
    display.setCurrent(MainForm);
    try
    {
      String Connectable = BluetoothServiceDiscovery.Scan(MainForm, "ACDC");
      if (Connectable != null)
      {
        MainForm.append("\n" + Connectable);
        display.setCurrent(MainForm);
        if (BluetoothCommunicator.Connect(MainForm, Connectable))
          Connect = CONNECTED;
        else
          Connect = NOT_CONNECTED;
        
        display.setCurrent(MainForm);
        BluetoothCommunicator.Handshake(MainForm, display);
        RecvThread = new BluetoothCommunicator(this, MainForm);
        
        display.setCurrent(MainForm);
      }
    }
    catch (Exception e)
    {
      MainForm.append("Error: Couldn't Search");
      Connect = NOT_CONNECTED;
      display.setCurrent(MainForm);
      return;
    }

    if (Connect == CONNECTED)
    {
      MainForm.addCommand(CMD_SHOWREMOTE);
      MainForm.addCommand(CMD_DISCONNECT);
      MainForm.append("Connected to a server now we start the receive thread\n");
      RecvThread.start();
    }
    else
    {
      MainForm.addCommand(CMD_CONNECT);
      MainForm.addCommand(CMD_EXIT);
      MainForm.append("Not connected to a server");
    }
    setCurrent();
  }

  public void startApp()
  {
    int length = ((255 << 8) | (255));
    System.out.println("Len " + length);
    
    
    MainForm = CreateForm("XBMC Remote");
    MainForm.append("Hi and welcome to the version " + Version + " of XBMC Remote.\nPressing Connect will have this remote connect to the first available Server");
    setCurrent();
  }

  private Form CreateForm(String Title)
  {
    Form rtnForm = new Form(Title);
    if (Connect == CONNECTED)
      rtnForm.addCommand(CMD_OK);
    else if (Connect == NOT_CONNECTED)
      rtnForm.addCommand(CMD_CONNECT);
    
    if (Connect == CONNECTED)
      rtnForm.addCommand(CMD_DISCONNECT);
    else if (Connect == NOT_CONNECTED)
      rtnForm.addCommand(CMD_EXIT);

    rtnForm.setCommandListener(this);
    return rtnForm;
  }

  private void setCurrent()
  {
    setCurrent(false);
  }
  private void setCurrent(boolean forceLog)
  {
    if (forceLog)
      display.setCurrent(MainForm);
    else if (Connect == CONNECTED)
    {
      display.setCurrent(keyHandler);
      keyHandler.repaint();
    }
    else
    {
      display.setCurrent(MainForm);
    }
  }

  public void pauseApp()
  {
  }

  public void destroyApp(boolean unconditional)
  {
    if (Connect == CONNECTED)
    {
      BluetoothCommunicator.Disconnect();
      RecvThread.close();
    }
  }

  public void commandAction(Command c, Displayable d)
  {
    if (c.equals(CMD_LOG))
      setCurrent(true);
    else if (c.equals(CMD_SHOWREMOTE))
    {
      display.setCurrent(keyHandler);
      keyHandler.repaint();
    }
    else if (d.equals(MainForm))
    {
      if      (c.equals(CMD_OK))
      {
        byte[] Packet = {'1', '1'}; // 48 == '0'
        BluetoothCommunicator.Send(Packet);
      }
      else if (c.equals(CMD_CONNECT))
      {
        this.Connect();
      }
      else if (c.equals(CMD_EXIT))
        System.out.println("QUIT");
      else if (c.equals(CMD_DISCONNECT))
      {
        this.Disconnect();
        this.RecvThread.close();
        setCurrent();
      }
    }
    else if (d.equals(keyHandler))
    {
      if (c.equals(CMD_OK))
      {
        byte[] Packet = {'2', (byte)251 };
        BluetoothCommunicator.Send(Packet);
      }
      else if (c.equals(CMD_DISCONNECT))
      {
        this.Disconnect();
        Connect = NOT_CONNECTED;
        setCurrent();
      }
    }
  }

  public void keyPressed(int keyCode)
  {
    byte[] Packet = {'2', (byte)keyCode};
    if (keyCode >= Canvas.KEY_NUM0 && keyCode <= Canvas.KEY_NUM9)
      Packet[1] = (byte)keyCode;
    
    else if (keyCode == Canvas.UP       || keyCode == -1)
      Packet[1] = 'U';
    else if (keyCode == Canvas.DOWN     || keyCode == -2)
      Packet[1] = 'D';
    else if (keyCode == Canvas.LEFT     || keyCode == -3)
      Packet[1] = 'L';
    else if (keyCode == Canvas.RIGHT    || keyCode == -4)
      Packet[1] = 'R';

    else if (keyCode == Canvas.KEY_STAR)
      Packet[1] = '*';
    else if (keyCode == Canvas.KEY_POUND)
      Packet[1] = '#';
    
    BluetoothCommunicator.Send(Packet);
    System.out.println("got" + keyCode + " I'll send [" + Packet[0] + "," + Packet[1] + "]");
  }
  public void Recv(int Header, byte[] Payload)
  {
    switch(Header)
    {
      case BluetoothEvent.RECV_COMMAND:
        break;
      case BluetoothEvent.RECV_SYSTEM:
        if (Payload[0] == 0)
          this.Disconnect();
        break;
      case BluetoothEvent.RECV_IMAGE:
        keyHandler.SetCover(Image.createImage(Payload, 0, Payload.length));
        break;
    }
  }
}

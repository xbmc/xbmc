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

import javax.microedition.lcdui.*;

public class BluetoothServiceDiscovery implements DiscoveryListener
{
	private static Object lock=new Object();
	private static Vector vecDevices=new Vector();
	private static String connectionURL=null;

	public static String Scan(Form Log, String ScanForUUID) throws Exception
  {
    BluetoothServiceDiscovery bluetoothServiceDiscovery=new BluetoothServiceDiscovery();
    //display local device address and name
    LocalDevice localDevice = LocalDevice.getLocalDevice();
    //find devices
    DiscoveryAgent agent = localDevice.getDiscoveryAgent();

    agent.startInquiry(DiscoveryAgent.GIAC, bluetoothServiceDiscovery);
    try 
    {
      synchronized(lock)
      { lock.wait(); }
    }
    catch (InterruptedException e)
    {
      e.printStackTrace();
    }
    Log.append("Device Inquiry Completed. \n");

    int deviceCount=vecDevices.size();
    if(deviceCount <= 0)
    {
      Log.append("No Devices Found .\n");
      return null;
    }
    else
    {
      Log.append("Bluetooth Devices: \n");
      for (int i = 0; i <deviceCount; i++)
      {
        RemoteDevice remoteDevice=(RemoteDevice)vecDevices.elementAt(i);
        String t = i + ": \"" + remoteDevice.getFriendlyName(true) + "\"";
        Log.append(t + "\n");
      }
    }
    UUID[] uuidSet = new UUID[1];
    if (ScanForUUID != null)
      uuidSet[0]=new UUID(ScanForUUID, true);
    else
      uuidSet[0]=new UUID("ACDC", true);

    for (int i = 0; i < vecDevices.capacity(); i++)
    {
      RemoteDevice remoteDevice=(RemoteDevice)vecDevices.elementAt(i);
      String t = remoteDevice.getFriendlyName(true);
      agent.searchServices(null,uuidSet,remoteDevice,bluetoothServiceDiscovery);
      try
      {
        synchronized(lock)
        { lock.wait(); }
      }
      catch (InterruptedException e)
      {
        e.printStackTrace();
      }
      if(connectionURL != null)
        return connectionURL;
    }
    Log.append("Couldn't find any computer that were suitable\n");
    return null;
	}

	public void deviceDiscovered(RemoteDevice btDevice, DeviceClass cod)
  {
		if(!vecDevices.contains(btDevice))
			vecDevices.addElement(btDevice);
	}

	public void servicesDiscovered(int transID, ServiceRecord[] servRecord)
  {
		if(servRecord!=null && servRecord.length>0){
			connectionURL=servRecord[0].getConnectionURL(0,false);
		}
		synchronized(lock)
    { lock.notify(); }
	}

	public void serviceSearchCompleted(int transID, int respCode)
  {
		synchronized(lock)
    { lock.notify(); }
	}

	public void inquiryCompleted(int discType)
  {
		synchronized(lock)
    { lock.notify(); }
	}
}

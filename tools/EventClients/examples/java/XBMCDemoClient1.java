package org.xbmc.eventclient.demo;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;

import org.xbmc.eventclient.XBMCClient;

/**
 * Simple Demo EventClient
 * @author Stefan Agner
 *
 */
public class XBMCDemoClient1 {

	/**
	 * Simple Demo EventClient
	 * @param args
	 */
	public static void main(String[] args) throws IOException, InterruptedException {
		InetAddress host = Inet4Address.getByAddress(new byte[] { (byte)192, (byte)168, 0, 20 } );

		Thread.sleep(20000);
		XBMCClient oXBMCClient = new XBMCClient(host, 9777, "My Client", "/usr/share/xbmc/media/icon.png");

		Thread.sleep(7000);

		oXBMCClient.sendNotification("My Title", "My Message");


		Thread.sleep(7000);

		oXBMCClient.sendButton("KB", "escape", false, true, false, (short)0 , (byte)0);


		Thread.sleep(7000);
		oXBMCClient.sendButton("KB", "escape", true, true, false, (short)0 , (byte)0);
		oXBMCClient.sendNotification("My Title", "Escape sent");

		Thread.sleep(1000);

		oXBMCClient.sendButton("KB", "escape", true, false, false, (short)0 , (byte)0);
		oXBMCClient.sendNotification("My Title", "Escape released");

		Thread.sleep(7000);
		oXBMCClient.sendLog((byte)0, "My Client disconnects....");
		oXBMCClient.sendNotification("My Title", "Client will disconnect");
		oXBMCClient.stopClient();

	}

}

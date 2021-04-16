using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using XBMC;

namespace XBMCEventClientDemo
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            EventClient eventClient = new EventClient();
            eventClient.Connect("127.0.0.1", 9777);

            string iconFile = @"../../icons/icon.png";
            IconType iconType = IconType.ICON_PNG;

            if !File.Exists(iconFile) {
                iconFile = @"/usr/share/xbmc/media/icon.png";
                if !File.Exists(iconFile) {
                    iconType = IconType.ICON_NONE;
                }
            }

            eventClient.SendHelo("XBMC Client Demo", iconType, iconFile);
            System.Threading.Thread.Sleep(1000);
            eventClient.SendNotification("XBMC Client Demo", "Notification Message", iconType, iconFile);
            System.Threading.Thread.Sleep(1000);
            eventClient.SendButton("dpadup", "XG", ButtonFlagsType.BTN_DOWN | ButtonFlagsType.BTN_NO_REPEAT, 0);
            System.Threading.Thread.Sleep(1000);
            eventClient.SendPing();
            System.Threading.Thread.Sleep(1000);
            eventClient.SendMouse(32768, 32768);
            System.Threading.Thread.Sleep(1000);
            eventClient.SendLog(LogTypeEnum.LOGERROR, "Example error log message from XBMC Client Demo");
            System.Threading.Thread.Sleep(1000);
            eventClient.SendAction("Mute");
            System.Threading.Thread.Sleep(1000);
            eventClient.SendBye();
        }
    }
}

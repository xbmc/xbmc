using System;
using System.Collections.Generic;
using System.Text;

namespace Platinum.Managed.SsdpTest
{
    class Program
    {
        static void Main(string[] args)
        {
            new Program().Run();
        }

        private void Run()
        {
            log4net.Config.BasicConfigurator.Configure();

            Console.Title = "Press 'q' to quit; 'l' to list devices";

            Console.WriteLine("Starting...");

            using (var upnp = new UPnP())
            {
                upnp.Start();

                var cp = new ControlPoint(true);

                cp.DeviceAdded += new ControlPoint.DeviceAddedDelegate(cp_DeviceAdded);
                cp.DeviceRemoved += new ControlPoint.DeviceRemovedDelegate(cp_DeviceRemoved);
                cp.ActionResponse += new ControlPoint.ActionResponseDelegate(cp_ActionResponse);

                upnp.AddControlPoint(cp);

                #region handle keyboard

                for (bool quit = false; !quit; )
                {
                    switch (Console.ReadKey(true).KeyChar)
                    {
                        case 'q':
                            quit = true;

                            break;

                        case 'l':
                            {
                                var devs = cp.Devices;

                                Console.WriteLine("Devices (" + devs.Length + "):");

                                foreach (var d in devs)
                                {
                                    Console.WriteLine("  " + d.FriendlyName);
                                }

                                break;
                            }
                    }
                }

                #endregion

                cp.DeviceAdded -= new ControlPoint.DeviceAddedDelegate(cp_DeviceAdded);
                cp.DeviceRemoved -= new ControlPoint.DeviceRemovedDelegate(cp_DeviceRemoved);

                upnp.Stop();
            }

            Console.WriteLine("Stopped.");
        }

        private void cp_DeviceRemoved(DeviceData dev)
        {
            Console.WriteLine("Removed: " + dev.FriendlyName);
        }

        private void cp_DeviceAdded(DeviceData dev)
        {
            Console.WriteLine("Added: " + dev.FriendlyName);
        }

        private void cp_ActionResponse(NeptuneException error, Action action)
        {
            Console.WriteLine("Action Response: " + action.Description + " (result = " + error.ErrorResult + ")");
        }
    }
}
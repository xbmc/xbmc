using System;
using System.Collections.Generic;
using System.Text;

namespace Platinum.Managed.MediaServerTest
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

            Console.Title = "Press 'q' to quit";

            Console.WriteLine("Starting...");

            using (var upnp = new UPnP())
            {
                var server = new MediaServer("TestManaged");
                server.BrowseMetadata += new MediaServer.BrowseMetadataDelegate(server_BrowseMetadata);
                server.BrowseDirectChildren += new MediaServer.BrowseDirectChildrenDelegate(server_BrowseDirectChildren);
                server.ProcessFileRequest += new MediaServer.ProcessFileRequestDelegate(server_ProcessFileRequest);
                
                upnp.AddDeviceHost(server);

                upnp.Start();

                #region handle keyboard

                for (bool quit = false; !quit; )
                {
                    switch (Console.ReadKey(true).KeyChar)
                    {
                        case 'q':
                            quit = true;

                            break;
                    }
                }

                #endregion

                server.BrowseMetadata -= new MediaServer.BrowseMetadataDelegate(server_BrowseMetadata);
                server.BrowseDirectChildren -= new MediaServer.BrowseDirectChildrenDelegate(server_BrowseDirectChildren);
                server.ProcessFileRequest -= new MediaServer.ProcessFileRequestDelegate(server_ProcessFileRequest);

                upnp.Stop();
            }

            Console.WriteLine("Stopped.");
        }

        private int server_BrowseMetadata(Action action, String object_id, String filter, Int32 starting_index, Int32 requested_count, String sort_criteria, HttpRequestContext context)
        {
            Console.WriteLine("BrowseMetadata: " + object_id);
            if (object_id == "0") 
            {
                var root = new MediaContainer();
                root.Title = "Root";
                root.ObjectID = "0";
                root.ParentID = "-1";
                root.Class = new ObjectClass("object.container.storageFolder", "");

                var didl = Didl.header + root.ToDidl(filter) + Didl.footer;
                action.SetArgumentValue("Result", didl);
                action.SetArgumentValue("NumberReturned", "1");
                action.SetArgumentValue("TotalMatches", "1");

                // update ID may be wrong here, it should be the one of the container?
                // TODO: We need to keep track of the overall updateID of the CDS
                action.SetArgumentValue("UpdateId", "1");

                return 0;
            }
            else if (object_id == "1")
            {
                var item = new MediaItem();
                item.Title = "Item";
                item.ObjectID = "1";
                item.ParentID = "0";
                item.Class = new ObjectClass("object.item.audioItem.musicTrack", "");

                var resource = new MediaResource();
                resource.ProtoInfo = ProtocolInfo.GetProtocolInfoFromMimeType("audio/mp3", true, context);

                // get list of ips and make sure the ip the request came from is used for the first resource returned
                // this ensures that clients which look only at the first resource will be able to reach the item
                List<String> ips = UPnP.GetIpAddresses(true);
                String localIP = context.LocalAddress.ip;
                if (localIP != "0.0.0.0")
                {
                    ips.Remove(localIP);
                    ips.Insert(0, localIP);
                }

                // iterate through all ips and create a resource for each
                foreach (String ip in ips)
                {
                    resource.URI = new Uri("http://" + ip + ":" + context.LocalAddress.port + "/test/test.mp3").ToString();
                    item.AddResource(resource);
                }
                                
                var didl = Didl.header + item.ToDidl(filter) + Didl.footer;
                action.SetArgumentValue("Result", didl);
                action.SetArgumentValue("NumberReturned", "1");
                action.SetArgumentValue("TotalMatches", "1");

                // update ID may be wrong here, it should be the one of the container?
                // TODO: We need to keep track of the overall updateID of the CDS
                action.SetArgumentValue("UpdateId", "1");

                return 0;
            }

            return -1;
        }


        private int server_BrowseDirectChildren(Action action, String object_id, String filter, Int32 starting_index, Int32 requested_count, String sort_criteria, HttpRequestContext context)
        {
            Console.WriteLine("BrowseDirectChildren: " + object_id);
            if (object_id != "0") return -1;

            var item = new MediaItem();
            item.Title = "Item";
            item.ObjectID = "1";
            item.ParentID = "0";
            item.Class = new ObjectClass("object.item.audioItem.musicTrack", "");

            var resource = new MediaResource();
            resource.ProtoInfo = ProtocolInfo.GetProtocolInfoFromMimeType("audio/mp3", true, context);

            // get list of ips and make sure the ip the request came from is used for the first resource returned
            // this ensures that clients which look only at the first resource will be able to reach the item
            List<String> ips = UPnP.GetIpAddresses(true);
            String localIP = context.LocalAddress.ip;
            if (localIP != "0.0.0.0")
            {
                ips.Remove(localIP);
                ips.Insert(0, localIP);
            }

            // iterate through all ips and create a resource for each
            foreach (String ip in ips)
            {
                resource.URI = new Uri("http://" + ip + ":" + context.LocalAddress.port + "/test/test.mp3").ToString();
                item.AddResource(resource);
            }

            var didl = Didl.header + item.ToDidl(filter) + Didl.footer;
            action.SetArgumentValue("Result", didl);
            action.SetArgumentValue("NumberReturned", "1");
            action.SetArgumentValue("TotalMatches", "1");

            // update ID may be wrong here, it should be the one of the container?
            // TODO: We need to keep track of the overall updateID of the CDS
            action.SetArgumentValue("UpdateId", "1");

            return 0;
        }

        private int server_ProcessFileRequest(HttpRequestContext context, HttpResponse response)
        {
            Uri uri = context.Request.URI;
            if (uri.AbsolutePath == "/test/test.mp3")
            {
                MediaServer.SetResponseFilePath(context, response, "C:\\Test.mp3");
                return 0;
            }

            return -1;
        }

    }
}

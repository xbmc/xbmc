#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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

namespace JSONRPC
{
  const char* const JSON_SERVICE_DESCRIPTION = 
  "{"
    "\"id\": \"http://www.xbmc.org/jsonrpc.jsd\","
    "\"version\": 3,"
    "\"description\": \"JSON RPC API of XBMC\","

    "\"JSONRPC.Introspect\": {"
        "\"type\": \"method\","
        "\"description\": \"Enumerates all actions and descriptions\","
        "\"transport\": \"Response\","
        "\"permission\": \"ReadData\","
        "\"statechanging\": false,"
        "\"params\": ["
            "{ \"name\": \"getdescriptions\", \"type\": \"boolean\", \"default\": true },"
            "{ \"name\": \"getmetadata\", \"type\": \"boolean\", \"default\": false },"
            "{ \"name\": \"filterbytransport\", \"type\": \"boolean\", \"default\": true }"
        "],"
        "\"returns\": \"object\""
    "},"
    "\"JSONRPC.Version\": {"
        "\"type\": \"method\","
        "\"description\": \"Retrieve the jsonrpc protocol version\","
        "\"transport\": \"Response\","
        "\"permission\": \"ReadData\","
        "\"statechanging\": false,"
        "\"params\": [],"
        "\"returns\": \"string\""
    "},"
    "\"JSONRPC.Permission\": {"
        "\"type\": \"method\","
        "\"description\": \"Retrieve the clients permissions\","
        "\"transport\": \"Response\","
        "\"permission\": \"ReadData\","
        "\"statechanging\": false,"
        "\"params\": [],"
        "\"returns\": {"
            "\"type\": \"object\","
            "\"properties\": {"
                "\"ReadData\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"ControlPlayback\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"ControlAnnounce\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"ControlPower\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"Logging\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"ScanLibrary\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" }"
            "}"
        "}"
    "},"
    "\"JSONRPC.Ping\": {"
        "\"type\": \"method\","
        "\"description\": \"Ping responder\","
        "\"transport\": \"Response\","
        "\"permission\": \"ReadData\","
        "\"statechanging\": false,"
        "\"params\": [],"
        "\"returns\": \"string\""
    "},"
    "\"JSONRPC.GetAnnouncementFlags\": {"
        "\"type\": \"method\","
        "\"description\": \"Get announcement flags\","
        "\"transport\": \"Announcing\","
        "\"permission\": \"ReadData\","
        "\"statechanging\": false,"
        "\"params\": [],"
        "\"returns\": {"
            "\"id\": \"Announcement.Flags\","
            "\"type\": \"object\","
            "\"properties\": {"
                "\"Playback\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"GUI\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"System\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"Library\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" },"
                "\"Other\": { \"type\": \"boolean\", \"required\": true, \"description\": \"\" }"
            "}"
        "}"
    "},"
    "\"JSONRPC.SetAnnouncementFlags\": {"
        "\"type\": \"method\","
        "\"description\": \"Change the announcement flags\","
        "\"transport\": \"Announcing\","
        "\"permission\": \"ControlAnnounce\","
        "\"statechanging\": true,"
        "\"params\": ["
            "{ \"name\": \"Playback\", \"type\": \"boolean\", \"default\": false },"
            "{ \"name\": \"GUI\", \"type\": \"boolean\", \"default\": false },"
            "{ \"name\": \"System\", \"type\": \"boolean\", \"default\": false },"
            "{ \"name\": \"Library\", \"type\": \"boolean\", \"default\": false },"
            "{ \"name\": \"Other\", \"type\": \"boolean\", \"default\": false }"
        "],"
        "\"returns\": { \"$ref\": \"Announcement.Flags\" }"
    "},"
    "\"JSONRPC.Announce\": {"
        "\"type\": \"method\","
        "\"description\": \"Announce to other connected clients\","
        "\"transport\": \"Response\","
        "\"permission\": \"ReadData\","
        "\"statechanging\": false,"
        "\"params\": ["
            "{ \"name\": \"sender\", \"type\": \"string\", \"required\": true },"
            "{ \"name\": \"message\", \"type\": \"string\", \"required\": true },"
            "{ \"name\": \"data\", \"type\": \"any\", \"default\": null }"
        "],"
        "\"returns\": \"any\""
    "}"
  "}";

  const char* const JSON_NOTIFICATION_DESCRIPTION =
  "{"

  "}";
}

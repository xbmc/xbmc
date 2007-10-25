/*****************************************************************
|
|   Platinum - AV Media Item
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMediaItem.h"
#include "PltMediaServer.h"
#include "PltMetadataHandler.h"
#include "PltDidl.h"
#include "PltXmlHelper.h"
#include "PltService.h"
#include "../MediaRenderer/PltMediaController.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.item")

extern const char* didl_namespace_dc;
extern const char* didl_namespace_upnp;

/*----------------------------------------------------------------------
|   PLT_PersonRoles::AddPerson
+---------------------------------------------------------------------*/
NPT_Result
PLT_PersonRoles::Add(const NPT_String& name, const NPT_String& role /*= ""*/)
{
  PLT_PersonRole person;
  person.name = name;
  person.role = role;
  return NPT_List<PLT_PersonRole>::Add(person);
}

/*----------------------------------------------------------------------
|   PLT_PersonRoles::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_PersonRoles::ToDidl(NPT_String& didl, const NPT_String& tag)
{
    for (NPT_List<PLT_PersonRole>::Iterator it = NPT_List<PLT_PersonRole>::GetFirstItem(); it; it++) {
        if(it->role.IsEmpty()) {
            didl += "<upnp:" + tag + ">";
        } else {
            didl += "<upnp:" + tag + " upnp:role=\"";
            PLT_Didl::AppendXmlEscape(didl, it->role);
            didl += "\">";
        }
        PLT_Didl::AppendXmlEscape(didl, it->name);
        didl += "</upnp:" + tag + ">";
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_PersonRoles::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_PersonRoles::FromDidl(const NPT_Array<NPT_XmlElementNode*>& nodes)
{
    for (NPT_Cardinal i = 0; i < nodes.GetItemCount(); i++) {
        PLT_PersonRole person;
        const NPT_String* name = nodes[i]->GetText();
        const NPT_String* role = nodes[i]->GetAttribute("role", didl_namespace_upnp);
        if (name) person.name = *name;
        if (role) person.role = *role;
        NPT_CHECK(NPT_List<PLT_PersonRole>::Add(person));
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaItemResource::PLT_MediaItemResource
+---------------------------------------------------------------------*/
PLT_MediaItemResource::PLT_MediaItemResource()
{
    m_Uri          = "";
    m_ProtocolInfo = "";
    m_Duration     = -1;
    m_Size         = -1;
    m_Protection   = "";
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetExtFromFilePath
+---------------------------------------------------------------------*/
const char*
PLT_MediaObject::GetExtFromFilePath(const NPT_String filepath, const char* dir_delimiter)
{
    /* first look for the '.' character */
    int ext_index = filepath.ReverseFind('.');
    if (ext_index <= 0 ) {
        return NULL;
    }

    /* then look for the dir delimiter index and make sure the '.' is found after the delimiter */
    int dir_delim_index = filepath.ReverseFind(dir_delimiter);
    if (dir_delim_index > 0 && ext_index < dir_delim_index) {
        return NULL;
    }

    return ((const char*)filepath) + ext_index;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetProtInfoFromExt
+---------------------------------------------------------------------*/
const char*
PLT_MediaObject::GetProtInfoFromExt(const char* ext)
{
    const char*      ret = NULL;
    NPT_String extension = ext;

    //TODO: we need to add more!
    if (extension.Compare(".mp3", true) == 0) {
        ret = "http-get:*:audio/mpeg:*";
    } else if (extension.Compare(".wma", true) == 0) {
        ret = "http-get:*:audio/x-ms-wma:*";
    } else {
        ret = "http-get:*:application/octet-stream:*";
    }

    return ret;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::GetUPnPClassFromExt
+---------------------------------------------------------------------*/
const char*
PLT_MediaObject::GetUPnPClassFromExt(const char* ext)
{
    const char*      ret = NULL;
    NPT_String extension = ext;

    if (extension.Compare(".mp3", true) == 0) {
        ret = "object.item.audioItem.musicTrack";
    } else if (extension.Compare(".wma", true) == 0) {
        ret = "object.item.audioItem.musicTrack";
    } else {
        ret = "object.item";
    }

    return ret;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::Reset
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaObject::Reset() 
{
    m_ObjectClass.type = "";
    m_ObjectClass.friendly_name = "";
    m_ObjectID = "";
    m_ParentID = "";

    m_Title = "";
    m_Creator = "";
    m_Restricted = true;

    m_People.actors.Clear();
    m_People.artists.Clear();    
    m_People.authors.Clear();

    m_Affiliation.album     = "";
    m_Affiliation.genre.Clear();
    m_Affiliation.playlist  = "";

    m_Description.description = "";
    m_Description.long_description = "";
    m_ExtraInfo.album_art_uri = "";
    m_ExtraInfo.artist_discography_uri = "";

    m_MiscInfo.original_track_number = 0;
    m_MiscInfo.dvdregioncode = 0;
    m_MiscInfo.toc = "";
    m_MiscInfo.user_annotation = "";

    m_Resources.Clear();

    m_Didl = "";

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaObject::ToDidl(NPT_UInt32 mask, NPT_String& didl)
{
    // title is required
    didl += "<dc:title>";
    PLT_Didl::AppendXmlEscape(didl, m_Title);
    didl += "</dc:title>";

    // class is required
    didl += "<upnp:class>";
    PLT_Didl::AppendXmlEscape(didl, m_ObjectClass.type);
    didl += "</upnp:class>";

    // creator
    if (mask & PLT_FILTER_MASK_CREATOR && m_Creator.GetLength() > 0) {
        didl += "<dc:creator>";
        PLT_Didl::AppendXmlEscape(didl, m_Creator);
        didl += "</dc:creator>";
    }

    // artist
    if (mask & PLT_FILTER_MASK_ARTIST) {
        m_People.artists.ToDidl(didl, "artist");
    }

    // actor
    if (mask & PLT_FILTER_MASK_ACTOR) {
        m_People.actors.ToDidl(didl, "actor");
    }

    // actor
    if (mask & PLT_FILTER_MASK_AUTHOR) {
        m_People.authors.ToDidl(didl, "author");
    }
    
    // album
    if (mask & PLT_FILTER_MASK_ALBUM && m_Affiliation.album.GetLength() > 0) {
        didl += "<upnp:album>";
        PLT_Didl::AppendXmlEscape(didl, m_Affiliation.album);
        didl += "</upnp:album>";
    }

    // genre
    if (mask & PLT_FILTER_MASK_GENRE) {
        for (NPT_List<NPT_String>::Iterator it = m_Affiliation.genre.GetFirstItem(); it; ++it) {
            didl += "<upnp:genre>";
            PLT_Didl::AppendXmlEscape(didl, (*it));
            didl += "</upnp:genre>";        
        }
    }

    // album art URI
    if (mask & PLT_FILTER_MASK_ALBUMARTURI && m_ExtraInfo.album_art_uri.GetLength() > 0) {
        if(m_ExtraInfo.album_art_uri_dlna_profile.GetLength() > 0) {
          didl += "<upnp:albumArtURI dlna:profileID=\"";
          didl += PLT_Didl::AppendXmlEscape(didl, m_ExtraInfo.album_art_uri_dlna_profile);
          didl += "\">";
        } else {
          didl += "<upnp:albumArtURI>";
        }
        PLT_Didl::AppendXmlEscape(didl, m_ExtraInfo.album_art_uri);
        didl += "</upnp:albumArtURI>";
    }

    // description
    if (mask & PLT_FILTER_MASK_DESCRIPTION && m_Description.long_description.GetLength() > 0) {
        didl += "<upnp:longDescription>";
        PLT_Didl::AppendXmlEscape(didl, m_Description.long_description);
        didl += "</upnp:longDescription>";
    }

    // original track number
    if (mask & PLT_FILTER_MASK_ORIGINALTRACK && m_MiscInfo.original_track_number > 0) {
        didl += "<upnp:originalTrackNumber>";
        didl += NPT_String::FromInteger(m_MiscInfo.original_track_number);
        didl += "</upnp:originalTrackNumber>";
    }

    // resource
    if (mask & PLT_FILTER_MASK_RES) {
        for (unsigned int i=0; i<m_Resources.GetItemCount(); i++) {
            if (m_Resources[i].m_ProtocolInfo.GetLength() > 0) {
                // protocol info is required
                didl += "<res protocolInfo=\"";
                PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_ProtocolInfo);

                if (mask & PLT_FILTER_MASK_RES_DURATION && m_Resources[i].m_Duration != -1) {
                    didl += "\" duration=\"";
                    PLT_Didl::FormatTimeStamp(didl, m_Resources[i].m_Duration);
                }

                if (mask & PLT_FILTER_MASK_RES_SIZE && m_Resources[i].m_Size != -1) {
                    didl += "\" size=\"";
                    didl += NPT_String::FromInteger(m_Resources[i].m_Size);
                }

                if (mask & PLT_FILTER_MASK_RES_PROTECTION && m_Resources[i].m_Protection.GetLength() > 0) {
                    didl += "\" protection=\"";
                    PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_Protection);
                }

                didl += "\">";
                PLT_Didl::AppendXmlEscape(didl, m_Resources[i].m_Uri);
                didl += "</res>";
            }
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaObject::FromDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaObject::FromDidl(NPT_XmlElementNode* entry)
{
    NPT_String str, xml;
    NPT_Array<NPT_XmlElementNode*> resources;
    NPT_Result res;

    // serialize the entry Didl as a we might need to pass it to a renderer
    res = PLT_XmlHelper::Serialize(*entry, xml);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);
    
    m_Didl = didl_header + xml + didl_footer;    

    // check if item is restricted (is default true?)
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(entry, "restricted", str))) {
        m_Restricted = PLT_Service::IsTrue(str);
    }

    res = PLT_XmlHelper::GetAttribute(entry, "id", m_ObjectID);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    res = PLT_XmlHelper::GetAttribute(entry, "parentID", m_ParentID);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    res = PLT_XmlHelper::GetChildText(entry, "title", m_Title, didl_namespace_dc);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    res = PLT_XmlHelper::GetChildText(entry, "class", m_ObjectClass.type, didl_namespace_upnp);
    NPT_CHECK_LABEL_SEVERE(res, cleanup);

    // read non-required elements
    PLT_XmlHelper::GetChildText(entry, "creator", m_Creator, didl_namespace_dc);

    PLT_XmlHelper::GetChildren(entry, resources, "artist", didl_namespace_upnp);
    m_People.artists.FromDidl(resources);

    PLT_XmlHelper::GetChildText(entry, "album", m_Affiliation.album, didl_namespace_upnp);

    resources.Clear();
    PLT_XmlHelper::GetChildren(entry, resources, "genre", didl_namespace_upnp);
    for (unsigned int i=0; i<resources.GetItemCount(); i++) {
        if(resources[i]->GetText()) {
            m_Affiliation.genre.Add(*resources[i]->GetText());
        }
    }

    PLT_XmlHelper::GetChildText(entry, "albumArtURI", m_ExtraInfo.album_art_uri, didl_namespace_upnp);
    PLT_XmlHelper::GetChildText(entry, "longDescription", m_Description.long_description, didl_namespace_upnp);
    PLT_XmlHelper::GetChildText(entry, "originalTrackNumber", str, didl_namespace_upnp);
    if( NPT_FAILED(str.ToInteger((long&)m_MiscInfo.original_track_number)) )
        m_MiscInfo.original_track_number = 0;

    resources.Clear();
    PLT_XmlHelper::GetChildren(entry, resources, "res");
    if (resources.GetItemCount() > 0) {
        for (unsigned int i=0; i<resources.GetItemCount(); i++) {
            PLT_MediaItemResource resource;
            if (resources[i]->GetText() == NULL) {
                goto cleanup;
            }

            resource.m_Uri = *resources[i]->GetText();
            if (NPT_FAILED(PLT_XmlHelper::GetAttribute(resources[i], "protocolInfo", resource.m_ProtocolInfo))) {
                goto cleanup;
            }

            if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(resources[i], "size", str))) {
                if (NPT_FAILED(str.ToInteger((long&)resource.m_Size))) {
                    // if error while converting, ignore and set to -1 to show we don't know the size
                    resource.m_Size = -1;
                }
            }

            if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(resources[i], "duration", str))) {
                if (NPT_FAILED(PLT_Didl::ParseTimeStamp(str, (NPT_UInt32&)resource.m_Duration))) {
                    // if error while converting, ignore and set to -1 to indicate we don't know the duration
                    resource.m_Duration = -1;
                }
            }    
            m_Resources.Add(resource);
        }
    }

    return NPT_SUCCESS;

cleanup:
    return res;
}

/*----------------------------------------------------------------------
|   PLT_MediaObjectList::PLT_MediaObjectList
+---------------------------------------------------------------------*/
PLT_MediaObjectList::PLT_MediaObjectList()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaObjectList::~PLT_MediaObjectList
+---------------------------------------------------------------------*/
PLT_MediaObjectList::~PLT_MediaObjectList()
{
    Apply(NPT_ObjectDeleter<PLT_MediaObject>());
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::PLT_MediaItem
+---------------------------------------------------------------------*/
PLT_MediaItem::PLT_MediaItem()
{
    Reset();
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::~PLT_MediaItem
+---------------------------------------------------------------------*/
PLT_MediaItem::~PLT_MediaItem()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaItem::ToDidl(NPT_UInt32 mask, NPT_String& didl)
{
    NPT_String tmp;
    // Allocate enough space for a big string we're going to concatenate in
    tmp.Reserve(2048);

    tmp = "<item id=\"";

    PLT_Didl::AppendXmlEscape(tmp, m_ObjectID);
    tmp += "\" parentID=\"";
    PLT_Didl::AppendXmlEscape(tmp, m_ParentID);
    tmp += "\" restricted=\"";
    tmp += m_Restricted?"1\"":"0\"";

    tmp += ">";

    NPT_CHECK_SEVERE(PLT_MediaObject::ToDidl(mask, tmp));

    /* close tag */
    tmp += "</item>";

    didl += tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaItem::FromDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaItem::FromDidl(NPT_XmlElementNode* entry)
{
    /* reset first */
    Reset();

    if (entry->GetTag().Compare("item", true) != 0)
        return NPT_ERROR_INTERNAL;

    return PLT_MediaObject::FromDidl(entry);
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::PLT_MediaContainer
+---------------------------------------------------------------------*/
PLT_MediaContainer::PLT_MediaContainer()
{
    Reset();
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::~PLT_MediaContainer
+---------------------------------------------------------------------*/
PLT_MediaContainer::~PLT_MediaContainer(void)
{
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::Reset
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaContainer::Reset() 
{
    m_SearchClasses.Clear();
    m_Searchable = true;
    m_ChildrenCount = -1;

    return PLT_MediaObject::Reset();
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaContainer::ToDidl(NPT_UInt32 mask, NPT_String& didl)
{
    NPT_String tmp;
    // Allocate enough space for a big string we're going to concatenate in
    tmp.Reserve(2048);

    tmp = "<container id=\"";

    PLT_Didl::AppendXmlEscape(tmp, m_ObjectID);
    tmp += "\" parentID=\"";
    PLT_Didl::AppendXmlEscape(tmp, m_ParentID);
    tmp += "\" restricted=\"";
    tmp += m_Restricted?"1\"":"0\"";

    if (mask & PLT_FILTER_MASK_SEARCHABLE) {
        tmp += " searchable=\"";
        tmp += m_Searchable?"1\"":"0\"";
    }
    
    if (mask & PLT_FILTER_MASK_CHILDCOUNT && m_ChildrenCount != -1) {
        tmp += " childCount=\"";
        tmp += NPT_String::FromInteger(m_ChildrenCount);
        tmp += "\"";
    }

    tmp += ">";

    NPT_CHECK_SEVERE(PLT_MediaObject::ToDidl(mask, tmp));

    /* close tag */
    tmp += "</container>";

    didl += tmp;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaContainer::FromDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaContainer::FromDidl(NPT_XmlElementNode* entry)
{
    NPT_String str;

    /* reset first */
    Reset();

    // check entry type
    if (entry->GetTag().Compare("Container", true) != 0) 
        return NPT_ERROR_INTERNAL;

    // check if item is searchable (is default true?)
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(entry, "searchable", str))) {
        m_Searchable = PLT_Service::IsTrue(str);
    }

    // look for childCount
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetAttribute(entry, "childCount", str))) {
        long count;
        NPT_CHECK_SEVERE(str.ToInteger(count));
        m_ChildrenCount = count;
    }

    return PLT_MediaObject::FromDidl(entry);
}

/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ServiceDescription.h"
#include "JSONServiceDescription.h"
#include "utils/log.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "JSONRPC.h"
#include "PlayerOperations.h"
#include "PlaylistOperations.h"
#include "FileOperations.h"
#include "AudioLibrary.h"
#include "VideoLibrary.h"
#include "GUIOperations.h"
#include "AddonsOperations.h"
#include "SystemOperations.h"
#include "InputOperations.h"
#include "XBMCOperations.h"
#include "ApplicationOperations.h"
#include "PVROperations.h"
#include "ProfilesOperations.h"
#include "FavouritesOperations.h"
#include "TextureOperations.h"
#include "SettingsOperations.h"

using namespace JSONRPC;

std::map<std::string, CVariant> CJSONServiceDescription::m_notifications = std::map<std::string, CVariant>();
CJSONServiceDescription::CJsonRpcMethodMap CJSONServiceDescription::m_actionMap;
std::map<std::string, JSONSchemaTypeDefinitionPtr> CJSONServiceDescription::m_types = std::map<std::string, JSONSchemaTypeDefinitionPtr>();
CJSONServiceDescription::IncompleteSchemaDefinitionMap CJSONServiceDescription::m_incompleteDefinitions = CJSONServiceDescription::IncompleteSchemaDefinitionMap();

JsonRpcMethodMap CJSONServiceDescription::m_methodMaps[] = {
// JSON-RPC
  { "JSONRPC.Introspect",                           CJSONRPC::Introspect },
  { "JSONRPC.Version",                              CJSONRPC::Version },
  { "JSONRPC.Permission",                           CJSONRPC::Permission },
  { "JSONRPC.Ping",                                 CJSONRPC::Ping },
  { "JSONRPC.GetConfiguration",                     CJSONRPC::GetConfiguration },
  { "JSONRPC.SetConfiguration",                     CJSONRPC::SetConfiguration },
  { "JSONRPC.NotifyAll",                            CJSONRPC::NotifyAll },

// Player
  { "Player.GetActivePlayers",                      CPlayerOperations::GetActivePlayers },
  { "Player.GetPlayers",                            CPlayerOperations::GetPlayers },
  { "Player.GetProperties",                         CPlayerOperations::GetProperties },
  { "Player.GetItem",                               CPlayerOperations::GetItem },

  { "Player.PlayPause",                             CPlayerOperations::PlayPause },
  { "Player.Stop",                                  CPlayerOperations::Stop },
  { "Player.SetSpeed",                              CPlayerOperations::SetSpeed },
  { "Player.Seek",                                  CPlayerOperations::Seek },
  { "Player.Move",                                  CPlayerOperations::Move },
  { "Player.Zoom",                                  CPlayerOperations::Zoom },
  { "Player.Rotate",                                CPlayerOperations::Rotate },
  
  { "Player.Open",                                  CPlayerOperations::Open },
  { "Player.GoTo",                                  CPlayerOperations::GoTo },
  { "Player.SetShuffle",                            CPlayerOperations::SetShuffle },
  { "Player.SetRepeat",                             CPlayerOperations::SetRepeat },
  { "Player.SetPartymode",                          CPlayerOperations::SetPartymode },
  
  { "Player.SetAudioStream",                        CPlayerOperations::SetAudioStream },
  { "Player.SetSubtitle",                           CPlayerOperations::SetSubtitle },
  { "Player.SetVideoStream",                        CPlayerOperations::SetVideoStream },

// Playlist
  { "Playlist.GetPlaylists",                        CPlaylistOperations::GetPlaylists },
  { "Playlist.GetProperties",                       CPlaylistOperations::GetProperties },
  { "Playlist.GetItems",                            CPlaylistOperations::GetItems },
  { "Playlist.Add",                                 CPlaylistOperations::Add },
  { "Playlist.Insert",                              CPlaylistOperations::Insert },
  { "Playlist.Clear",                               CPlaylistOperations::Clear },
  { "Playlist.Remove",                              CPlaylistOperations::Remove },
  { "Playlist.Swap",                                CPlaylistOperations::Swap },

// Files
  { "Files.GetSources",                             CFileOperations::GetRootDirectory },
  { "Files.GetDirectory",                           CFileOperations::GetDirectory },
  { "Files.GetFileDetails",                         CFileOperations::GetFileDetails },
  { "Files.SetFileDetails",                         CFileOperations::SetFileDetails },
  { "Files.PrepareDownload",                        CFileOperations::PrepareDownload },
  { "Files.Download",                               CFileOperations::Download },

// Music Library
  { "AudioLibrary.GetProperties",                   CAudioLibrary::GetProperties },
  { "AudioLibrary.GetArtists",                      CAudioLibrary::GetArtists },
  { "AudioLibrary.GetArtistDetails",                CAudioLibrary::GetArtistDetails },
  { "AudioLibrary.GetAlbums",                       CAudioLibrary::GetAlbums },
  { "AudioLibrary.GetAlbumDetails",                 CAudioLibrary::GetAlbumDetails },
  { "AudioLibrary.GetSongs",                        CAudioLibrary::GetSongs },
  { "AudioLibrary.GetSongDetails",                  CAudioLibrary::GetSongDetails },
  { "AudioLibrary.GetRecentlyAddedAlbums",          CAudioLibrary::GetRecentlyAddedAlbums },
  { "AudioLibrary.GetRecentlyAddedSongs",           CAudioLibrary::GetRecentlyAddedSongs },
  { "AudioLibrary.GetRecentlyPlayedAlbums",         CAudioLibrary::GetRecentlyPlayedAlbums },
  { "AudioLibrary.GetRecentlyPlayedSongs",          CAudioLibrary::GetRecentlyPlayedSongs },
  { "AudioLibrary.GetGenres",                       CAudioLibrary::GetGenres },
  { "AudioLibrary.GetRoles",                        CAudioLibrary::GetRoles },
  { "AudioLibrary.SetArtistDetails",                CAudioLibrary::SetArtistDetails },
  { "AudioLibrary.SetAlbumDetails",                 CAudioLibrary::SetAlbumDetails },
  { "AudioLibrary.SetSongDetails",                  CAudioLibrary::SetSongDetails },
  { "AudioLibrary.Scan",                            CAudioLibrary::Scan },
  { "AudioLibrary.Export",                          CAudioLibrary::Export },
  { "AudioLibrary.Clean",                           CAudioLibrary::Clean },

// Video Library
  { "VideoLibrary.GetGenres",                       CVideoLibrary::GetGenres },
  { "VideoLibrary.GetTags",                         CVideoLibrary::GetTags },
  { "VideoLibrary.GetMovies",                       CVideoLibrary::GetMovies },
  { "VideoLibrary.GetMovieDetails",                 CVideoLibrary::GetMovieDetails },
  { "VideoLibrary.GetMovieSets",                    CVideoLibrary::GetMovieSets },
  { "VideoLibrary.GetMovieSetDetails",              CVideoLibrary::GetMovieSetDetails },
  { "VideoLibrary.GetTVShows",                      CVideoLibrary::GetTVShows },
  { "VideoLibrary.GetTVShowDetails",                CVideoLibrary::GetTVShowDetails },
  { "VideoLibrary.GetSeasons",                      CVideoLibrary::GetSeasons },
  { "VideoLibrary.GetSeasonDetails",                CVideoLibrary::GetSeasonDetails },
  { "VideoLibrary.GetEpisodes",                     CVideoLibrary::GetEpisodes },
  { "VideoLibrary.GetEpisodeDetails",               CVideoLibrary::GetEpisodeDetails },
  { "VideoLibrary.GetMusicVideos",                  CVideoLibrary::GetMusicVideos },
  { "VideoLibrary.GetMusicVideoDetails",            CVideoLibrary::GetMusicVideoDetails },
  { "VideoLibrary.GetRecentlyAddedMovies",          CVideoLibrary::GetRecentlyAddedMovies },
  { "VideoLibrary.GetRecentlyAddedEpisodes",        CVideoLibrary::GetRecentlyAddedEpisodes },
  { "VideoLibrary.GetRecentlyAddedMusicVideos",     CVideoLibrary::GetRecentlyAddedMusicVideos },
  { "VideoLibrary.GetInProgressTVShows",            CVideoLibrary::GetInProgressTVShows },
  { "VideoLibrary.SetMovieDetails",                 CVideoLibrary::SetMovieDetails },
  { "VideoLibrary.SetMovieSetDetails",              CVideoLibrary::SetMovieSetDetails },
  { "VideoLibrary.SetTVShowDetails",                CVideoLibrary::SetTVShowDetails },
  { "VideoLibrary.SetSeasonDetails",                CVideoLibrary::SetSeasonDetails },
  { "VideoLibrary.SetEpisodeDetails",               CVideoLibrary::SetEpisodeDetails },
  { "VideoLibrary.SetMusicVideoDetails",            CVideoLibrary::SetMusicVideoDetails },
  { "VideoLibrary.RefreshMovie",                    CVideoLibrary::RefreshMovie },
  { "VideoLibrary.RefreshTVShow",                   CVideoLibrary::RefreshTVShow },
  { "VideoLibrary.RefreshEpisode",                  CVideoLibrary::RefreshEpisode },
  { "VideoLibrary.RefreshMusicVideo",               CVideoLibrary::RefreshMusicVideo },
  { "VideoLibrary.RemoveMovie",                     CVideoLibrary::RemoveMovie },
  { "VideoLibrary.RemoveTVShow",                    CVideoLibrary::RemoveTVShow },
  { "VideoLibrary.RemoveEpisode",                   CVideoLibrary::RemoveEpisode },
  { "VideoLibrary.RemoveMusicVideo",                CVideoLibrary::RemoveMusicVideo },
  { "VideoLibrary.Scan",                            CVideoLibrary::Scan },
  { "VideoLibrary.Export",                          CVideoLibrary::Export },
  { "VideoLibrary.Clean",                           CVideoLibrary::Clean },
  
// Addon operations
  { "Addons.GetAddons",                             CAddonsOperations::GetAddons },
  { "Addons.GetAddonDetails",                       CAddonsOperations::GetAddonDetails },
  { "Addons.SetAddonEnabled",                       CAddonsOperations::SetAddonEnabled },
  { "Addons.ExecuteAddon",                          CAddonsOperations::ExecuteAddon },

// GUI operations
  { "GUI.GetProperties",                            CGUIOperations::GetProperties },
  { "GUI.ActivateWindow",                           CGUIOperations::ActivateWindow },
  { "GUI.ShowNotification",                         CGUIOperations::ShowNotification },
  { "GUI.SetFullscreen",                            CGUIOperations::SetFullscreen },
  { "GUI.SetStereoscopicMode",                      CGUIOperations::SetStereoscopicMode },
  { "GUI.GetStereoscopicModes",                     CGUIOperations::GetStereoscopicModes },

// PVR operations
  { "PVR.GetProperties",                            CPVROperations::GetProperties },
  { "PVR.GetChannelGroups",                         CPVROperations::GetChannelGroups },
  { "PVR.GetChannelGroupDetails",                   CPVROperations::GetChannelGroupDetails },
  { "PVR.GetChannels",                              CPVROperations::GetChannels },
  { "PVR.GetChannelDetails",                        CPVROperations::GetChannelDetails },
  { "PVR.GetBroadcasts",                            CPVROperations::GetBroadcasts },
  { "PVR.GetBroadcastDetails",                      CPVROperations::GetBroadcastDetails },
  { "PVR.GetTimers",                                CPVROperations::GetTimers },
  { "PVR.GetTimerDetails",                          CPVROperations::GetTimerDetails },
  { "PVR.GetRecordings",                            CPVROperations::GetRecordings },
  { "PVR.GetRecordingDetails",                      CPVROperations::GetRecordingDetails },
  { "PVR.AddTimer",                                 CPVROperations::AddTimer },
  { "PVR.DeleteTimer",                              CPVROperations::DeleteTimer },
  { "PVR.ToggleTimer",                              CPVROperations::ToggleTimer },
  { "PVR.Record",                                   CPVROperations::Record },
  { "PVR.Scan",                                     CPVROperations::Scan },

// Profiles operations
  { "Profiles.GetProfiles",                         CProfilesOperations::GetProfiles},
  { "Profiles.GetCurrentProfile",                   CProfilesOperations::GetCurrentProfile},
  { "Profiles.LoadProfile",                         CProfilesOperations::LoadProfile},

// System operations
  { "System.GetProperties",                         CSystemOperations::GetProperties },
  { "System.EjectOpticalDrive",                     CSystemOperations::EjectOpticalDrive },
  { "System.Shutdown",                              CSystemOperations::Shutdown },
  { "System.Suspend",                               CSystemOperations::Suspend },
  { "System.Hibernate",                             CSystemOperations::Hibernate },
  { "System.Reboot",                                CSystemOperations::Reboot },

// Input operations
  { "Input.SendText",                               CInputOperations::SendText },
  { "Input.ExecuteAction",                          CInputOperations::ExecuteAction },
  { "Input.Left",                                   CInputOperations::Left },
  { "Input.Right",                                  CInputOperations::Right },
  { "Input.Down",                                   CInputOperations::Down },
  { "Input.Up",                                     CInputOperations::Up },
  { "Input.Select",                                 CInputOperations::Select },
  { "Input.Back",                                   CInputOperations::Back },
  { "Input.ContextMenu",                            CInputOperations::ContextMenu },
  { "Input.Info",                                   CInputOperations::Info },
  { "Input.Home",                                   CInputOperations::Home },
  { "Input.ShowCodec",                              CInputOperations::ShowCodec },
  { "Input.ShowOSD",                                CInputOperations::ShowOSD },
  { "Input.ShowPlayerProcessInfo",                  CInputOperations::ShowPlayerProcessInfo },

// Application operations
  { "Application.GetProperties",                    CApplicationOperations::GetProperties },
  { "Application.SetVolume",                        CApplicationOperations::SetVolume },
  { "Application.SetMute",                          CApplicationOperations::SetMute },
  { "Application.Quit",                             CApplicationOperations::Quit },

// Favourites operations
  { "Favourites.GetFavourites",                     CFavouritesOperations::GetFavourites },
  { "Favourites.AddFavourite",                      CFavouritesOperations::AddFavourite },

// Textures operations
  { "Textures.GetTextures",                         CTextureOperations::GetTextures },
  { "Textures.RemoveTexture",                       CTextureOperations::RemoveTexture },

// Settings operations
  { "Settings.GetSections",                         CSettingsOperations::GetSections },
  { "Settings.GetCategories",                       CSettingsOperations::GetCategories },
  { "Settings.GetSettings",                         CSettingsOperations::GetSettings },
  { "Settings.GetSettingValue",                     CSettingsOperations::GetSettingValue },
  { "Settings.SetSettingValue",                     CSettingsOperations::SetSettingValue },
  { "Settings.ResetSettingValue",                   CSettingsOperations::ResetSettingValue },

// XBMC operations
  { "XBMC.GetInfoLabels",                           CXBMCOperations::GetInfoLabels },
  { "XBMC.GetInfoBooleans",                         CXBMCOperations::GetInfoBooleans }
};

JSONSchemaTypeDefinition::JSONSchemaTypeDefinition()
  : missingReference(),
    name(),
    ID(),
    referencedType(nullptr),
    referencedTypeSet(false),
    extends(),
    description(),
    type(AnyValue),
    unionTypes(),
    optional(true),
    defaultValue(),
    minimum(-std::numeric_limits<double>::max()),
    maximum(std::numeric_limits<double>::max()),
    exclusiveMinimum(false),
    exclusiveMaximum(false),
    divisibleBy(0),
    minLength(-1),
    maxLength(-1),
    enums(),
    items(),
    minItems(0),
    maxItems(0),
    uniqueItems(false),
    additionalItems(),
    properties(),
    hasAdditionalProperties(false),
    additionalProperties(nullptr)
{ }

bool JSONSchemaTypeDefinition::Parse(const CVariant &value, bool isParameter /* = false */)
{
  bool hasReference = false;

  // Check if the type of the parameter defines a json reference
  // to a type defined somewhere else
  if (value.isMember("$ref") && value["$ref"].isString())
  {
    // Get the name of the referenced type
    std::string refType = value["$ref"].asString();
    // Check if the referenced type exists
    JSONSchemaTypeDefinitionPtr referencedTypeDef = CJSONServiceDescription::GetType(refType);
    if (refType.length() <= 0 || referencedTypeDef.get() == NULL)
    {
      CLog::Log(LOGDEBUG, "JSONRPC: JSON schema type %s references an unknown type %s", name.c_str(), refType.c_str());
      missingReference = refType;
      return false;
    }
    
    std::string typeName = name;
    *this = *referencedTypeDef;
    if (!typeName.empty())
      name = typeName;
    referencedType = referencedTypeDef;
    hasReference = true;
  }
  else if (value.isMember("id") && value["id"].isString())
    ID = GetString(value["id"], "");

  // Check if the "required" field has been defined
  optional = value.isMember("required") && value["required"].isBoolean() ? !value["required"].asBoolean() : true;

  // Get the "description"
  if (!hasReference || (value.isMember("description") && value["description"].isString()))
    description = GetString(value["description"], "");

  if (hasReference)
  {
    // If there is a specific default value, read it
    if (value.isMember("default") && IsType(value["default"], type))
    {
      bool ok = false;
      if (enums.size() <= 0)
        ok = true;
      // If the type has an enum definition we must make
      // sure that the default value is a valid enum value
      else
      {
        for (std::vector<CVariant>::const_iterator itr = enums.begin(); itr != enums.end(); ++itr)
        {
          if (value["default"] == *itr)
          {
            ok = true;
            break;
          }
        }
      }

      if (ok)
        defaultValue = value["default"];
    }

    return true;
  }

  // Check whether this type extends an existing type
  if (value.isMember("extends"))
  {
    if (value["extends"].isString())
    {
      std::string extendsName = GetString(value["extends"], "");
      if (!extendsName.empty())
      {
        JSONSchemaTypeDefinitionPtr extendedTypeDef = CJSONServiceDescription::GetType(extendsName);
        if (extendedTypeDef.get() == NULL)
        {
          CLog::Log(LOGDEBUG, "JSONRPC: JSON schema type %s extends an unknown type %s", name.c_str(), extendsName.c_str());
          missingReference = extendsName;
          return false;
        }

        type = extendedTypeDef->type;
        extends.push_back(extendedTypeDef);
      }
    }
    else if (value["extends"].isArray())
    {
      JSONSchemaType extendedType = AnyValue;
      for (unsigned int extendsIndex = 0; extendsIndex < value["extends"].size(); extendsIndex++)
      {
        std::string extendsName = GetString(value["extends"][extendsIndex], "");
        if (!extendsName.empty())
        {
          JSONSchemaTypeDefinitionPtr extendedTypeDef = CJSONServiceDescription::GetType(extendsName);
          if (extendedTypeDef.get() == NULL)
          {
            extends.clear();
            CLog::Log(LOGDEBUG, "JSONRPC: JSON schema type %s extends an unknown type %s", name.c_str(), extendsName.c_str());
            missingReference = extendsName;
            return false;
          }

          if (extendsIndex == 0)
            extendedType = extendedTypeDef->type;
          else if (extendedType != extendedTypeDef->type)
          {
            extends.clear();
            CLog::Log(LOGDEBUG, "JSONRPC: JSON schema type %s extends multiple JSON schema types of mismatching types", name.c_str());
            return false;
          }

          extends.push_back(extendedTypeDef);
        }
      }

      type = extendedType;
    }
  }

  // Only read the "type" attribute if it's
  // not an extending type
  if (extends.size() <= 0)
  {
    // Get the defined type of the parameter
    if (!CJSONServiceDescription::parseJSONSchemaType(value["type"], unionTypes, type, missingReference))
      return false;
  }

  if (HasType(type, ObjectValue))
  {
    // If the type definition is of type "object"
    // and has a "properties" definition we need
    // to handle these as well
    if (value.isMember("properties") && value["properties"].isObject())
    {
      // Get all child elements of the "properties"
      // object and loop through them
      for (CVariant::const_iterator_map itr = value["properties"].begin_map(); itr != value["properties"].end_map(); ++itr)
      {
        // Create a new type definition, store the name
        // of the current property into it, parse it
        // recursively and add its default value
        // to the current type's default value
        JSONSchemaTypeDefinitionPtr propertyType = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
        propertyType->name = itr->first;
        if (!propertyType->Parse(itr->second))
        {
          missingReference = propertyType->missingReference;
          return false;
        }
        defaultValue[itr->first] = propertyType->defaultValue;
        properties.add(propertyType);
      }
    }

    hasAdditionalProperties = true;
    additionalProperties = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
    if (value.isMember("additionalProperties"))
    {
      if (value["additionalProperties"].isBoolean())
      {
        hasAdditionalProperties = value["additionalProperties"].asBoolean();
        if (!hasAdditionalProperties)
        {
          additionalProperties.reset();
        }
      }
      else if (value["additionalProperties"].isObject() && !value["additionalProperties"].isNull())
      {
        if (!additionalProperties->Parse(value["additionalProperties"]))
        {
          missingReference = additionalProperties->missingReference;
          hasAdditionalProperties = false;
          additionalProperties.reset();
          
          CLog::Log(LOGDEBUG, "JSONRPC: Invalid additionalProperties schema definition in type %s", name.c_str());
          return false;
        }
      }
      else
      {
        CLog::Log(LOGDEBUG, "JSONRPC: Invalid additionalProperties definition in type %s", name.c_str());
        return false;
      }
    }
  }

  // If the defined parameter is an array
  // we need to check for detailed definitions
  // of the array items
  if (HasType(type, ArrayValue))
  {
    // Check for "uniqueItems" field
    if (value.isMember("uniqueItems") && value["uniqueItems"].isBoolean())
      uniqueItems = value["uniqueItems"].asBoolean();
    else
      uniqueItems = false;

    // Check for "additionalItems" field
    if (value.isMember("additionalItems"))
    {
      // If it is an object, there is only one schema for it
      if (value["additionalItems"].isObject())
      {
        JSONSchemaTypeDefinitionPtr additionalItem = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
        if (additionalItem->Parse(value["additionalItems"]))
          additionalItems.push_back(additionalItem);
        else
        {
          CLog::Log(LOGDEBUG, "Invalid \"additionalItems\" value for type %s", name.c_str());
          missingReference = additionalItem->missingReference;
          return false;
        }
      }
      // If it is an array there may be multiple schema definitions
      else if (value["additionalItems"].isArray())
      {
        for (unsigned int itemIndex = 0; itemIndex < value["additionalItems"].size(); itemIndex++)
        {
          JSONSchemaTypeDefinitionPtr additionalItem = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());

          if (additionalItem->Parse(value["additionalItems"][itemIndex]))
            additionalItems.push_back(additionalItem);
          else
          {
            CLog::Log(LOGDEBUG, "Invalid \"additionalItems\" value (item %d) for type %s", itemIndex, name.c_str());
            missingReference = additionalItem->missingReference;
            return false;
          }
        }
      }
      // If it is not a (array of) schema and not a bool (default value is false)
      // it has an invalid value
      else if (!value["additionalItems"].isBoolean())
      {
        CLog::Log(LOGDEBUG, "Invalid \"additionalItems\" definition for type %s", name.c_str());
        return false;
      }
    }

    // If the "items" field is a single object
    // we can parse that directly
    if (value.isMember("items"))
    {
      if (value["items"].isObject())
      {
        JSONSchemaTypeDefinitionPtr item = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
        if (!item->Parse(value["items"]))
        {
          CLog::Log(LOGDEBUG, "Invalid item definition in \"items\" for type %s", name.c_str());
          missingReference = item->missingReference;
          return false;
        }
        items.push_back(item);
      }
      // Otherwise if it is an array we need to
      // parse all elements and store them
      else if (value["items"].isArray())
      {
        for (CVariant::const_iterator_array itemItr = value["items"].begin_array(); itemItr != value["items"].end_array(); ++itemItr)
        {
          JSONSchemaTypeDefinitionPtr item = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
          if (!item->Parse(*itemItr))
          {
            CLog::Log(LOGDEBUG, "Invalid item definition in \"items\" array for type %s", name.c_str());
            missingReference = item->missingReference;
            return false;
          }
          items.push_back(item);
        }
      }
    }

    minItems = (unsigned int)value["minItems"].asUnsignedInteger(0);
    maxItems = (unsigned int)value["maxItems"].asUnsignedInteger(0);
  }

  if (HasType(type, NumberValue) || HasType(type, IntegerValue))
  {
    if ((type & NumberValue) == NumberValue)
    {
      minimum = value["minimum"].asDouble(-std::numeric_limits<double>::max());
      maximum = value["maximum"].asDouble(std::numeric_limits<double>::max());
    }
    else if ((type  & IntegerValue) == IntegerValue)
    {
      minimum = (double)value["minimum"].asInteger(std::numeric_limits<int>::min());
      maximum = (double)value["maximum"].asInteger(std::numeric_limits<int>::max());
    }

    exclusiveMinimum = value["exclusiveMinimum"].asBoolean(false);
    exclusiveMaximum = value["exclusiveMaximum"].asBoolean(false);
    divisibleBy = (unsigned int)value["divisibleBy"].asUnsignedInteger(0);
  }
      
  if (HasType(type, StringValue))
  {
    minLength = (int)value["minLength"].asInteger(-1);
    maxLength = (int)value["maxLength"].asInteger(-1);
  }

  // If the type definition is neither an
  // "object" nor an "array" we can check
  // for an "enum" definition
  if (value.isMember("enum") && value["enum"].isArray())
  {
    // Loop through all elements in the "enum" array
    for (CVariant::const_iterator_array enumItr = value["enum"].begin_array(); enumItr != value["enum"].end_array(); ++enumItr)
    {
      // Check for duplicates and eliminate them
      bool approved = true;
      for (unsigned int approvedIndex = 0; approvedIndex < enums.size(); approvedIndex++)
      {
        if (*enumItr == enums.at(approvedIndex))
        {
          approved = false;
          break;
        }
      }

      // Only add the current item to the enum value 
      // list if it is not duplicate
      if (approved)
        enums.push_back(*enumItr);
    }
  }

  if (type != ObjectValue)
  {
    // If there is a definition for a default value and its type
    // matches the type of the parameter we can parse it
    bool ok = false;
    if (value.isMember("default") && IsType(value["default"], type))
    {
      if (enums.size() <= 0)
        ok = true;
      // If the type has an enum definition we must make
      // sure that the default value is a valid enum value
      else
      {
        for (std::vector<CVariant>::const_iterator itr = enums.begin(); itr != enums.end(); ++itr)
        {
          if (value["default"] == *itr)
          {
            ok = true;
            break;
          }
        }
      }
    }

    if (ok)
      defaultValue = value["default"];
    else
    {
      // If the type of the default value definition does not
      // match the type of the parameter we have to log this
      if (value.isMember("default") && !IsType(value["default"], type))
        CLog::Log(LOGDEBUG, "JSONRPC: Parameter %s has an invalid default value", name.c_str());
      
      // If the type contains an "enum" we need to get the
      // default value from the first enum value
      if (enums.size() > 0)
        defaultValue = enums.at(0);
      // otherwise set a default value instead
      else
        SetDefaultValue(defaultValue, type);
    }
  }

  return true;
}

JSONRPC_STATUS JSONSchemaTypeDefinition::Check(const CVariant &value, CVariant &outputValue, CVariant &errorData)
{
  if (!name.empty())
    errorData["name"] = name;
  SchemaValueTypeToJson(type, errorData["type"]);
  std::string errorMessage;

  if (referencedType != NULL && !referencedTypeSet)
    Set(referencedType);

  // Let's check the type of the provided parameter
  if (!IsType(value, type))
  {
    errorMessage = StringUtils::Format("Invalid type %s received", ValueTypeToString(value.type()));
    errorData["message"] = errorMessage.c_str();
    return InvalidParams;
  }
  else if (value.isNull() && !HasType(type, NullValue))
  {
    errorData["message"] = "Received value is null";
    return InvalidParams;
  }

  // Let's check if we have to handle a union type
  if (unionTypes.size() > 0)
  {
    bool ok = false;
    for (unsigned int unionIndex = 0; unionIndex < unionTypes.size(); unionIndex++)
    {
      CVariant dummyError;
      CVariant testOutput = outputValue;
      if (unionTypes.at(unionIndex)->Check(value, testOutput, dummyError) == OK)
      {
        ok = true;
        outputValue = testOutput;
        break;
      }
    }

    if (!ok)
    {
      errorData["message"] = "Received value does not match any of the union type definitions";
      return InvalidParams;
    }
  }

  // First we need to check if this type extends another
  // type and if so we need to check against the extended
  // type first
  if (extends.size() > 0)
  {
    for (unsigned int extendsIndex = 0; extendsIndex < extends.size(); extendsIndex++)
    {
      JSONRPC_STATUS status = extends.at(extendsIndex)->Check(value, outputValue, errorData);

      if (status != OK)
      {
        CLog::Log(LOGDEBUG, "JSONRPC: Value does not match extended type %s of type %s", extends.at(extendsIndex)->ID.c_str(), name.c_str());
        errorMessage = StringUtils::Format("value does not match extended type %s", extends.at(extendsIndex)->ID.c_str());
        errorData["message"] = errorMessage.c_str();
        return status;
      }
    }
  }

  // If it is an array we need to
  // - check the type of every element ("items")
  // - check if they need to be unique ("uniqueItems")
  if (HasType(type, ArrayValue) && value.isArray())
  {
    outputValue = CVariant(CVariant::VariantTypeArray);
    // Check the number of items against minItems and maxItems
    if ((minItems > 0 && value.size() < minItems) || (maxItems > 0 && value.size() > maxItems))
    {
      CLog::Log(LOGDEBUG, "JSONRPC: Number of array elements does not match minItems and/or maxItems in type %s", name.c_str());
      if (minItems > 0 && maxItems > 0)
        errorMessage = StringUtils::Format("Between %d and %d array items expected but %d received", minItems, maxItems, value.size());
      else if (minItems > 0)
        errorMessage = StringUtils::Format("At least %d array items expected but only %d received", minItems, value.size());
      else
        errorMessage = StringUtils::Format("Only %d array items expected but %d received", maxItems, value.size());
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }

    if (items.size() == 0)
      outputValue = value;
    else if (items.size() == 1)
    {
      JSONSchemaTypeDefinitionPtr itemType = items.at(0);

      // Loop through all array elements
      for (unsigned int arrayIndex = 0; arrayIndex < value.size(); arrayIndex++)
      {
        CVariant temp;
        JSONRPC_STATUS status = itemType->Check(value[arrayIndex], temp, errorData["property"]);
        outputValue.push_back(temp);
        if (status != OK)
        {
          CLog::Log(LOGDEBUG, "JSONRPC: Array element at index %u does not match in type %s", arrayIndex, name.c_str());
          errorMessage = StringUtils::Format("array element at index %u does not match", arrayIndex);
          errorData["message"] = errorMessage.c_str();
          return status;
        }
      }
    }
    // We have more than one element in "items"
    // so we have tuple typing, which means that
    // every element in the value array must match
    // with the type at the same position in the
    // "items" array
    else
    {
      // If the number of elements in the value array
      // does not match the number of elements in the
      // "items" array and additional items are not
      // allowed there is no need to check every element
      if (value.size() < items.size() || (value.size() != items.size() && additionalItems.size() == 0))
      {
        CLog::Log(LOGDEBUG, "JSONRPC: One of the array elements does not match in type %s", name.c_str());
        errorMessage = StringUtils::Format("%" PRIuS" array elements expected but %d received", items.size(), value.size());
        errorData["message"] = errorMessage.c_str();
        return InvalidParams;
      }

      // Loop through all array elements until there
      // are either no more schemas in the "items"
      // array or no more elements in the value's array
      unsigned int arrayIndex;
      for (arrayIndex = 0; arrayIndex < std::min(items.size(), (size_t)value.size()); arrayIndex++)
      {
        JSONRPC_STATUS status = items.at(arrayIndex)->Check(value[arrayIndex], outputValue[arrayIndex], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGDEBUG, "JSONRPC: Array element at index %u does not match with items schema in type %s", arrayIndex, name.c_str());
          return status;
        }
      }

      if (additionalItems.size() > 0)
      {
        // Loop through the rest of the elements
        // in the array and check them against the
        // "additionalItems"
        for (; arrayIndex < value.size(); arrayIndex++)
        {
          bool ok = false;
          for (unsigned int additionalIndex = 0; additionalIndex < additionalItems.size(); additionalIndex++)
          {
            CVariant dummyError;
            if (additionalItems.at(additionalIndex)->Check(value[arrayIndex], outputValue[arrayIndex], dummyError) == OK)
            {
              ok = true;
              break;
            }
          }

          if (!ok)
          {
            CLog::Log(LOGDEBUG, "JSONRPC: Array contains non-conforming additional items in type %s", name.c_str());
            errorMessage = StringUtils::Format("Array element at index %u does not match the \"additionalItems\" schema", arrayIndex);
            errorData["message"] = errorMessage.c_str();
            return InvalidParams;
          }
        }
      }
    }

    // If every array element is unique we need to check each one
    if (uniqueItems)
    {
      for (unsigned int checkingIndex = 0; checkingIndex < outputValue.size(); checkingIndex++)
      {
        for (unsigned int checkedIndex = checkingIndex + 1; checkedIndex < outputValue.size(); checkedIndex++)
        {
          // If two elements are the same they are not unique
          if (outputValue[checkingIndex] == outputValue[checkedIndex])
          {
            CLog::Log(LOGDEBUG, "JSONRPC: Not unique array element at index %u and %u in type %s", checkingIndex, checkedIndex, name.c_str());
            errorMessage = StringUtils::Format("Array element at index %u is not unique (same as array element at index %u)", checkingIndex, checkedIndex);
            errorData["message"] = errorMessage.c_str();
            return InvalidParams;
          }
        }
      }
    }

    return OK;
  }

  // If it is an object we need to check every element
  // against the defined "properties"
  if (HasType(type, ObjectValue) && value.isObject())
  {
    unsigned int handled = 0;
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesEnd = properties.end();
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesIterator;
    for (propertiesIterator = properties.begin(); propertiesIterator != propertiesEnd; ++propertiesIterator)
    {
      if (value.isMember(propertiesIterator->second->name))
      {
        JSONRPC_STATUS status = propertiesIterator->second->Check(value[propertiesIterator->second->name], outputValue[propertiesIterator->second->name], errorData["property"]);
        if (status != OK)
        {
          CLog::Log(LOGDEBUG, "JSONRPC: Invalid property \"%s\" in type %s", propertiesIterator->second->name.c_str(), name.c_str());
          return status;
        }
        handled++;
      }
      else if (propertiesIterator->second->optional)
        outputValue[propertiesIterator->second->name] = propertiesIterator->second->defaultValue;
      else
      {
        errorData["property"]["name"] = propertiesIterator->second->name.c_str();
        errorData["property"]["type"] = SchemaValueTypeToString(propertiesIterator->second->type);
        errorData["message"] = "Missing property";
        return InvalidParams;
      }
    }

    // Additional properties are not allowed
    if (handled < value.size())
    {
      // If additional properties are allowed we need to check if
      // they match the defined schema
      if (hasAdditionalProperties && additionalProperties != NULL)
      {
        CVariant::const_iterator_map iter;
        CVariant::const_iterator_map iterEnd = value.end_map();
        for (iter = value.begin_map(); iter != iterEnd; iter++)
        {
          if (properties.find(iter->first) != properties.end())
            continue;

          // If the additional property is of type "any"
          // we can simply copy its value to the output
          // object
          if (additionalProperties->type == AnyValue)
          {
            outputValue[iter->first] = value[iter->first];
            continue;
          }

          JSONRPC_STATUS status = additionalProperties->Check(value[iter->first], outputValue[iter->first], errorData["property"]);
          if (status != OK)
          {
            CLog::Log(LOGDEBUG, "JSONRPC: Invalid additional property \"%s\" in type %s", iter->first.c_str(), name.c_str());
            return status;
          }
        }
      }
      // If we still have unchecked properties but additional
      // properties are not allowed, we have invalid parameters
      else if (!hasAdditionalProperties || additionalProperties == NULL)
      {
        errorData["message"] = "Unexpected additional properties received";
        errorData.erase("property");
        return InvalidParams;
      }
    }

    return OK;
  }

  // It's neither an array nor an object

  // If it can only take certain values ("enum")
  // we need to check against those
  if (enums.size() > 0)
  {
    bool valid = false;
    for (std::vector<CVariant>::const_iterator enumItr = enums.begin(); enumItr != enums.end(); ++enumItr)
    {
      if (*enumItr == value)
      {
        valid = true;
        break;
      }
    }

    if (!valid)
    {
      CLog::Log(LOGDEBUG, "JSONRPC: Value does not match any of the enum values in type %s", name.c_str());
      errorData["message"] = "Received value does not match any of the defined enum values";
      return InvalidParams;
    }
  }

  // If we have a number or an integer type, we need
  // to check the minimum and maximum values
  if ((HasType(type, NumberValue) && value.isDouble()) || (HasType(type, IntegerValue) && value.isInteger()))
  {
    double numberValue;
    if (value.isDouble())
      numberValue = value.asDouble();
    else
      numberValue = (double)value.asInteger();
    // Check minimum
    if ((exclusiveMinimum && numberValue <= minimum) || (!exclusiveMinimum && numberValue < minimum) ||
    // Check maximum
        (exclusiveMaximum && numberValue >= maximum) || (!exclusiveMaximum && numberValue > maximum))        
    {
      CLog::Log(LOGDEBUG, "JSONRPC: Value does not lay between minimum and maximum in type %s", name.c_str());
      if (value.isDouble())
        errorMessage = StringUtils::Format("Value between %f (%s) and %f (%s) expected but %f received", 
          minimum, exclusiveMinimum ? "exclusive" : "inclusive", maximum, exclusiveMaximum ? "exclusive" : "inclusive", numberValue);
      else
        errorMessage = StringUtils::Format("Value between %d (%s) and %d (%s) expected but %d received", 
          (int)minimum, exclusiveMinimum ? "exclusive" : "inclusive", (int)maximum, exclusiveMaximum ? "exclusive" : "inclusive", (int)numberValue);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
    // Check divisibleBy
    if ((HasType(type, IntegerValue) && divisibleBy > 0 && ((int)numberValue % divisibleBy) != 0))
    {
      CLog::Log(LOGDEBUG, "JSONRPC: Value does not meet divisibleBy requirements in type %s", name.c_str());
      errorMessage = StringUtils::Format("Value should be divisible by %d but %d received", divisibleBy, (int)numberValue);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
  }

  // If we have a string, we need to check the length
  if (HasType(type, StringValue) && value.isString())
  {
    int size = value.asString().size();
    if (size < minLength)
    {
      CLog::Log(LOGDEBUG, "JSONRPC: Value does not meet minLength requirements in type %s", name.c_str());
      errorMessage = StringUtils::Format("Value should have a minimum length of %d but has a length of %d", minLength, size);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }

    if (maxLength >= 0 && size > maxLength)
    {
      CLog::Log(LOGDEBUG, "JSONRPC: Value does not meet maxLength requirements in type %s", name.c_str());
      errorMessage = StringUtils::Format("Value should have a maximum length of %d but has a length of %d", maxLength, size);
      errorData["message"] = errorMessage.c_str();
      return InvalidParams;
    }
  }

  // Otherwise it can have any value
  outputValue = value;
  return OK;
}

void JSONSchemaTypeDefinition::Print(bool isParameter, bool isGlobal, bool printDefault, bool printDescriptions, CVariant &output) const
{
  bool typeReference = false;

  // Printing general fields
  if (isParameter)
    output["name"] = name;

  if (isGlobal)
    output["id"] = ID;
  else if (!ID.empty())
  {
    output["$ref"] = ID;
    typeReference = true;
  }

  if (printDescriptions && !description.empty())
    output["description"] = description;

  if (isParameter || printDefault)
  {
    if (!optional)
      output["required"] = true;
    if (optional && type != ObjectValue && type != ArrayValue)
      output["default"] = defaultValue;
  }

  if (!typeReference)
  {
    if (extends.size() == 1)
    {
      output["extends"] = extends.at(0)->ID;
    }
    else if (extends.size() > 1)
    {
      output["extends"] = CVariant(CVariant::VariantTypeArray);
      for (unsigned int extendsIndex = 0; extendsIndex < extends.size(); extendsIndex++)
        output["extends"].append(extends.at(extendsIndex)->ID);
    }
    else if (unionTypes.size() > 0)
    {
      output["type"] = CVariant(CVariant::VariantTypeArray);
      for (unsigned int unionIndex = 0; unionIndex < unionTypes.size(); unionIndex++)
      {
        CVariant unionOutput = CVariant(CVariant::VariantTypeObject);
        unionTypes.at(unionIndex)->Print(false, false, false, printDescriptions, unionOutput);
        output["type"].append(unionOutput);
      }
    }
    else
      CJSONUtils::SchemaValueTypeToJson(type, output["type"]);

    // Printing enum field
    if (enums.size() > 0)
    {
      output["enums"] = CVariant(CVariant::VariantTypeArray);
      for (unsigned int enumIndex = 0; enumIndex < enums.size(); enumIndex++)
        output["enums"].append(enums.at(enumIndex));
    }

    // Printing integer/number fields
    if (CJSONUtils::HasType(type, IntegerValue) || CJSONUtils::HasType(type, NumberValue))
    {
      if (CJSONUtils::HasType(type, NumberValue))
      {
        if (minimum > -std::numeric_limits<double>::max())
          output["minimum"] = minimum;
        if (maximum < std::numeric_limits<double>::max())
          output["maximum"] = maximum;
      }
      else
      {
        if (minimum > std::numeric_limits<int>::min())
          output["minimum"] = (int)minimum;
        if (maximum < std::numeric_limits<int>::max())
          output["maximum"] = (int)maximum;
      }

      if (exclusiveMinimum)
        output["exclusiveMinimum"] = true;
      if (exclusiveMaximum)
        output["exclusiveMaximum"] = true;
      if (divisibleBy > 0)
        output["divisibleBy"] = divisibleBy;
    }
    if (CJSONUtils::HasType(type, StringValue))
    {
      if (minLength >= 0)
        output["minLength"] = minLength;
      if (maxLength >= 0)
        output["maxLength"] = maxLength;
    }

    // Print array fields
    if (CJSONUtils::HasType(type, ArrayValue))
    {
      if (items.size() == 1)
      {
        items.at(0)->Print(false, false, false, printDescriptions, output["items"]);
      }
      else if (items.size() > 1)
      {
        output["items"] = CVariant(CVariant::VariantTypeArray);
        for (unsigned int itemIndex = 0; itemIndex < items.size(); itemIndex++)
        {
          CVariant item = CVariant(CVariant::VariantTypeObject);
          items.at(itemIndex)->Print(false, false, false, printDescriptions, item);
          output["items"].append(item);
        }
      }

      if (minItems > 0)
        output["minItems"] = minItems;
      if (maxItems > 0)
        output["maxItems"] = maxItems;

      if (additionalItems.size() == 1)
      {
        additionalItems.at(0)->Print(false, false, false, printDescriptions, output["additionalItems"]);
      }
      else if (additionalItems.size() > 1)
      {
        output["additionalItems"] = CVariant(CVariant::VariantTypeArray);
        for (unsigned int addItemIndex = 0; addItemIndex < additionalItems.size(); addItemIndex++)
        {
          CVariant item = CVariant(CVariant::VariantTypeObject);
          additionalItems.at(addItemIndex)->Print(false, false, false, printDescriptions, item);
          output["additionalItems"].append(item);
        }
      }

      if (uniqueItems)
        output["uniqueItems"] = true;
    }

    // Print object fields
    if (CJSONUtils::HasType(type, ObjectValue))
    {
      if (properties.size() > 0)
      {
        output["properties"] = CVariant(CVariant::VariantTypeObject);

        JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesEnd = properties.end();
        JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator propertiesIterator;
        for (propertiesIterator = properties.begin(); propertiesIterator != propertiesEnd; ++propertiesIterator)
        {
          propertiesIterator->second->Print(false, false, true, printDescriptions, output["properties"][propertiesIterator->first]);
        }
      }

      if (!hasAdditionalProperties)
        output["additionalProperties"] = false;
      else if (additionalProperties != NULL && additionalProperties->type != AnyValue)
        additionalProperties->Print(false, false, true, printDescriptions, output["additionalProperties"]);
    }
  }
}

void JSONSchemaTypeDefinition::Set(const JSONSchemaTypeDefinitionPtr typeDefinition)
{
  if (typeDefinition.get() == NULL)
    return;

  std::string origName = name;
  std::string origDescription = description;
  bool origOptional = optional;
  CVariant origDefaultValue = defaultValue;
  JSONSchemaTypeDefinitionPtr referencedTypeDef = referencedType;

  // set all the values from the given type definition
  *this = *typeDefinition;

  // restore the original values
  if (!origName.empty())
    name = origName;

  if (!origDescription.empty())
    description = origDescription;

  if (!origOptional)
    optional = origOptional;

  if (!origDefaultValue.isNull())
    defaultValue = origDefaultValue;

  if (referencedTypeDef.get() != NULL)
    referencedType = referencedTypeDef;

  referencedTypeSet = true;
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::CJsonSchemaPropertiesMap() :
   m_propertiesmap(std::map<std::string, JSONSchemaTypeDefinitionPtr>())
{
}

void JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::add(JSONSchemaTypeDefinitionPtr property)
{
  std::string name = property->name;
  StringUtils::ToLower(name);
  m_propertiesmap[name] = property;
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::begin() const
{
  return m_propertiesmap.begin();
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::find(const std::string& key) const
{
  return m_propertiesmap.find(key);
}

JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::end() const
{
  return m_propertiesmap.end();
}

unsigned int JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::size() const
{
  return m_propertiesmap.size();
}

JsonRpcMethod::JsonRpcMethod()
  : missingReference(),
    name(),
    method(NULL),
    transportneed(Response),
    permission(ReadData),
    description(),
    parameters(),
    returns(new JSONSchemaTypeDefinition())
{ }

bool JsonRpcMethod::Parse(const CVariant &value)
{
  // Parse XBMC specific information about the method
  if (value.isMember("transport") && value["transport"].isArray())
  {
    int transport = 0;
    for (unsigned int index = 0; index < value["transport"].size(); index++)
      transport |= StringToTransportLayer(value["transport"][index].asString());

    transportneed = (TransportLayerCapability)transport;
  }
  else
    transportneed = StringToTransportLayer(value.isMember("transport") ? value["transport"].asString() : "");

  if (value.isMember("permission") && value["permission"].isArray())
  {
    int permissions = 0;
    for (unsigned int index = 0; index < value["permission"].size(); index++)
      permissions |= StringToPermission(value["permission"][index].asString());

    permission = (OperationPermission)permissions;
  }
  else
    permission = StringToPermission(value.isMember("permission") ? value["permission"].asString() : "");

  description = GetString(value["description"], "");

  // Check whether there are parameters defined
  if (value.isMember("params") && value["params"].isArray())
  {
    // Loop through all defined parameters
    for (unsigned int paramIndex = 0; paramIndex < value["params"].size(); paramIndex++)
    {
      CVariant parameter = value["params"][paramIndex];
      // If the parameter definition does not contain a valid "name" or
      // "type" element we will ignore it
      if (!parameter.isMember("name") || !parameter["name"].isString() ||
         (!parameter.isMember("type") && !parameter.isMember("$ref") && !parameter.isMember("extends")) ||
         (parameter.isMember("type") && !parameter["type"].isString() && !parameter["type"].isArray()) || 
         (parameter.isMember("$ref") && !parameter["$ref"].isString()) ||
         (parameter.isMember("extends") && !parameter["extends"].isString() && !parameter["extends"].isArray()))
      {
        CLog::Log(LOGDEBUG, "JSONRPC: Method %s has a badly defined parameter", name.c_str());
        return false;
      }

      // Parse the parameter and add it to the list
      // of defined parameters
      JSONSchemaTypeDefinitionPtr param = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
      if (!parseParameter(parameter, param))
      {
        missingReference = param->missingReference;
        return false;
      }
      parameters.push_back(param);
    }
  }
    
  // Parse the return value of the method
  if (!parseReturn(value))
  {
    missingReference = returns->missingReference;
    return false;
  }

  return true;
}

JSONRPC_STATUS JsonRpcMethod::Check(const CVariant &requestParameters, ITransportLayer *transport, IClient *client, bool notification, MethodCall &methodCall, CVariant &outputParameters) const
{
  if (transport != NULL && (transport->GetCapabilities() & transportneed) == transportneed)
  {
    if (client != NULL && (client->GetPermissionFlags() & permission) == permission && (!notification || (permission & OPERATION_PERMISSION_NOTIFICATION) == permission))
    {
      methodCall = method;

      // Count the number of actually handled (present)
      // parameters
      unsigned int handled = 0;
      CVariant errorData = CVariant(CVariant::VariantTypeObject);
      errorData["method"] = name;

      // Loop through all the parameters to check
      for (unsigned int i = 0; i < parameters.size(); i++)
      {
        // Evaluate the current parameter
        JSONRPC_STATUS status = checkParameter(requestParameters, parameters.at(i), i, outputParameters, handled, errorData);
        if (status != OK)
        {
          // Return the error data object in the outputParameters reference
          outputParameters = errorData;
          return status;
        }
      }

      // Check if there were unnecessary parameters
      if (handled < requestParameters.size())
      {
        errorData["message"] = "Too many parameters";
        outputParameters = errorData;
        return InvalidParams;
      }

      return OK;
    }
    else
      return BadPermission;
  }
  
  return MethodNotFound;
}

bool JsonRpcMethod::parseParameter(const CVariant &value, JSONSchemaTypeDefinitionPtr parameter)
{
  parameter->name = GetString(value["name"], "");

  // Parse the type and default value of the parameter
  return parameter->Parse(value, true);
}

bool JsonRpcMethod::parseReturn(const CVariant &value)
{
  // Only parse the "returns" definition if there is one
  if (!value.isMember("returns"))
  {
    returns->type = NullValue;
    return true;
  }
  
  // If the type of the return value is defined as a simple string we can parse it directly
  if (value["returns"].isString())
    return CJSONServiceDescription::parseJSONSchemaType(value["returns"], returns->unionTypes, returns->type, missingReference);
  
  // otherwise we have to parse the whole type definition
  if (!returns->Parse(value["returns"]))
  {
    missingReference = returns->missingReference;
    return false;
  }
  
  return true;
}

JSONRPC_STATUS JsonRpcMethod::checkParameter(const CVariant &requestParameters, JSONSchemaTypeDefinitionPtr type, unsigned int position, CVariant &outputParameters, unsigned int &handled, CVariant &errorData)
{
  // Let's check if the parameter has been provided
  if (ParameterExists(requestParameters, type->name, position))
  {
    // Get the parameter
    CVariant parameterValue = GetParameter(requestParameters, type->name, position);

    // Evaluate the type of the parameter
    JSONRPC_STATUS status = type->Check(parameterValue, outputParameters[type->name], errorData["stack"]);
    if (status != OK)
      return status;

    // The parameter was present and valid
    handled++;
  }
  // If the parameter has not been provided but is optional
  // we can use its default value
  else if (type->optional)
    outputParameters[type->name] = type->defaultValue;
  // The parameter is required but has not been provided => invalid
  else
  {
    errorData["stack"]["name"] = type->name;
    SchemaValueTypeToJson(type->type, errorData["stack"]["type"]);
    errorData["stack"]["message"] = "Missing parameter";
    return InvalidParams;
  }

  return OK;
}

void CJSONServiceDescription::Cleanup()
{
  // reset all of the static data
  m_notifications.clear();
  m_actionMap.clear();
  m_types.clear();
  m_incompleteDefinitions.clear();
}

bool CJSONServiceDescription::prepareDescription(std::string &description, CVariant &descriptionObject, std::string &name)
{
  if (description.empty())
  {
    CLog::Log(LOGERROR, "JSONRPC: Missing JSON Schema definition for \"%s\"", name.c_str());
    return false;
  }

  if (description.at(0) != '{')
  {
    description = StringUtils::Format("{%s}", description.c_str());
  }

  descriptionObject = CJSONVariantParser::Parse((const unsigned char *)description.c_str(), description.size());

  // Make sure the method description actually exists and represents an object
  if (!descriptionObject.isObject())
  {
    CLog::Log(LOGERROR, "JSONRPC: Unable to parse JSON Schema definition for \"%s\"", name.c_str());
    return false;
  }

  CVariant::const_iterator_map member = descriptionObject.begin_map();
  if (member != descriptionObject.end_map())
    name = member->first;

  if (name.empty() ||
     (!descriptionObject[name].isMember("type") && !descriptionObject[name].isMember("$ref") && !descriptionObject[name].isMember("extends")))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for \"%s\"", name.c_str());
    return false;
  }

  return true;
}

bool CJSONServiceDescription::addMethod(const std::string &jsonMethod, MethodCall method)
{
  CVariant descriptionObject;
  std::string methodName;

  std::string modJsonMethod = jsonMethod;
  // Make sure the method description actually exists and represents an object
  if (!prepareDescription(modJsonMethod, descriptionObject, methodName))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for method \"%s\"", methodName.c_str());
    return false;
  }

  if (m_actionMap.find(methodName) != m_actionMap.end())
  {
    CLog::Log(LOGERROR, "JSONRPC: There already is a method with the name \"%s\"", methodName.c_str());
    return false;
  }

  std::string type = GetString(descriptionObject[methodName]["type"], "");
  if (type.compare("method") != 0)
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON type for method \"%s\"", methodName.c_str());
    return false;
  }

  if (method == NULL)
  {
    unsigned int size = sizeof(m_methodMaps) / sizeof(JsonRpcMethodMap);
    for (unsigned int index = 0; index < size; index++)
    {
      if (methodName.compare(m_methodMaps[index].name) == 0)
      {
        method = m_methodMaps[index].method;
        break;
      }
    }

    if (method == NULL)
    {
      CLog::Log(LOGERROR, "JSONRPC: Missing implementation for method \"%s\"", methodName.c_str());
      return false;
    }
  }

  // Parse the details of the method
  JsonRpcMethod newMethod;
  newMethod.name = methodName;
  newMethod.method = method;
  
  if (!newMethod.Parse(descriptionObject[newMethod.name]))
  {
    CLog::Log(LOGERROR, "JSONRPC: Could not parse method \"%s\"", methodName.c_str());
    if (!newMethod.missingReference.empty())
    {
      IncompleteSchemaDefinition incomplete;
      incomplete.Schema = modJsonMethod;
      incomplete.Type = SchemaDefinitionMethod;
      incomplete.Method = method;
      
      IncompleteSchemaDefinitionMap::iterator iter = m_incompleteDefinitions.find(newMethod.missingReference);
      if (iter == m_incompleteDefinitions.end())
        m_incompleteDefinitions[newMethod.missingReference] = std::vector<IncompleteSchemaDefinition>();
      
      CLog::Log(LOGINFO, "JSONRPC: Adding method \"%s\" to list of incomplete definitions (waiting for \"%s\")", methodName.c_str(), newMethod.missingReference.c_str());
      m_incompleteDefinitions[newMethod.missingReference].push_back(incomplete);
    }
    
    return false;
  }

  m_actionMap.add(newMethod);

  return true;
}

bool CJSONServiceDescription::AddType(const std::string &jsonType)
{
  CVariant descriptionObject;
  std::string typeName;

  std::string modJsonType = jsonType;
  if (!prepareDescription(modJsonType, descriptionObject, typeName))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for type \"%s\"", typeName.c_str());
    return false;
  }

  if (m_types.find(typeName) != m_types.end())
  {
    CLog::Log(LOGERROR, "JSONRPC: There already is a type with the name \"%s\"", typeName.c_str());
    return false;
  }

  // Make sure the "id" attribute is correctly populated
  descriptionObject[typeName]["id"] = typeName;

  JSONSchemaTypeDefinitionPtr globalType = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
  globalType->name = typeName;
  globalType->ID = typeName;
  CJSONServiceDescription::addReferenceTypeDefinition(globalType);

  if (!globalType->Parse(descriptionObject[typeName]))
  {
    CLog::Log(LOGWARNING, "JSONRPC: Could not parse type \"%s\"", typeName.c_str());
    CJSONServiceDescription::removeReferenceTypeDefinition(typeName);
    if (!globalType->missingReference.empty())
    {
      IncompleteSchemaDefinition incomplete;
      incomplete.Schema = modJsonType;
      incomplete.Type = SchemaDefinitionType;
      
      IncompleteSchemaDefinitionMap::iterator iter = m_incompleteDefinitions.find(globalType->missingReference);
      if (iter == m_incompleteDefinitions.end())
        m_incompleteDefinitions[globalType->missingReference] = std::vector<IncompleteSchemaDefinition>();
      
      CLog::Log(LOGINFO, "JSONRPC: Adding type \"%s\" to list of incomplete definitions (waiting for \"%s\")", typeName.c_str(), globalType->missingReference.c_str());
      m_incompleteDefinitions[globalType->missingReference].push_back(incomplete);
    }

    globalType.reset();
    
    return false;
  }

  return true;
}

bool CJSONServiceDescription::AddMethod(const std::string &jsonMethod, MethodCall method)
{
  if (method == NULL)
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSONRPC method implementation");
    return false;
  }

  return addMethod(jsonMethod, method);
}

bool CJSONServiceDescription::AddBuiltinMethod(const std::string &jsonMethod)
{
  return addMethod(jsonMethod, NULL);
}

bool CJSONServiceDescription::AddNotification(const std::string &jsonNotification)
{
  CVariant descriptionObject;
  std::string notificationName;

  std::string modJsonNotification = jsonNotification;
  // Make sure the notification description actually exists and represents an object
  if (!prepareDescription(modJsonNotification, descriptionObject, notificationName))
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON Schema definition for notification \"%s\"", notificationName.c_str());
    return false;
  }

  if (m_notifications.find(notificationName) != m_notifications.end())
  {
    CLog::Log(LOGERROR, "JSONRPC: There already is a notification with the name \"%s\"", notificationName.c_str());
    return false;
  }

  std::string type = GetString(descriptionObject[notificationName]["type"], "");
  if (type.compare("notification") != 0)
  {
    CLog::Log(LOGERROR, "JSONRPC: Invalid JSON type for notification \"%s\"", notificationName.c_str());
    return false;
  }

  m_notifications[notificationName] = descriptionObject;

  return true;
}

bool CJSONServiceDescription::AddEnum(const std::string &name, const std::vector<CVariant> &values, CVariant::VariantType type /* = CVariant::VariantTypeNull */, const CVariant &defaultValue /* = CVariant::ConstNullVariant */)
{
  if (name.empty() || m_types.find(name) != m_types.end() ||
      values.size() == 0)
    return false;

  JSONSchemaTypeDefinitionPtr definition = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
  definition->ID = name;

  std::vector<CVariant::VariantType> types;
  bool autoType = false;
  if (type == CVariant::VariantTypeNull)
    autoType = true;
  else
    types.push_back(type);

  for (unsigned int index = 0; index < values.size(); index++)
  {
    if (autoType)
      types.push_back(values[index].type());
    else if (type != CVariant::VariantTypeConstNull && type != values[index].type())
      return false;
  }
  definition->enums.insert(definition->enums.begin(), values.begin(), values.end());

  int schemaType = (int)AnyValue;
  for (unsigned int index = 0; index < types.size(); index++)
  {
    JSONSchemaType currentType;
    switch (type)
    {
      case CVariant::VariantTypeString:
        currentType = StringValue;
        break;
      case CVariant::VariantTypeDouble:
        currentType = NumberValue;
        break;
      case CVariant::VariantTypeInteger:
      case CVariant::VariantTypeUnsignedInteger:
        currentType = IntegerValue;
        break;
      case CVariant::VariantTypeBoolean:
        currentType = BooleanValue;
        break;
      case CVariant::VariantTypeArray:
        currentType = ArrayValue;
        break;
      case CVariant::VariantTypeObject:
        currentType = ObjectValue;
        break;
      case CVariant::VariantTypeConstNull:
        currentType = AnyValue;
        break;
      default:
      case CVariant::VariantTypeNull:
        return false;
    }

    if (index == 0)
      schemaType = currentType;
    else
      schemaType |= (int)currentType;
  }
  definition->type = (JSONSchemaType)schemaType;
  
  if (defaultValue.type() == CVariant::VariantTypeConstNull)
    definition->defaultValue = definition->enums.at(0);
  else
    definition->defaultValue = defaultValue;

  addReferenceTypeDefinition(definition);

  return true;
}

bool CJSONServiceDescription::AddEnum(const std::string &name, const std::vector<std::string> &values)
{
  std::vector<CVariant> enums;
  for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
    enums.push_back(CVariant(*it));

  return AddEnum(name, enums, CVariant::VariantTypeString);
}

bool CJSONServiceDescription::AddEnum(const std::string &name, const std::vector<int> &values)
{
  std::vector<CVariant> enums;
  for (std::vector<int>::const_iterator it = values.begin(); it != values.end(); ++it)
    enums.push_back(CVariant(*it));

  return AddEnum(name, enums, CVariant::VariantTypeInteger);
}

const char* CJSONServiceDescription::GetVersion()
{
  return JSONRPC_SERVICE_VERSION;
}

JSONRPC_STATUS CJSONServiceDescription::Print(CVariant &result, ITransportLayer *transport, IClient *client,
  bool printDescriptions /* = true */, bool printMetadata /* = false */, bool filterByTransport /* = true */,
  const std::string &filterByName /* = "" */, const std::string &filterByType /* = "" */, bool printReferences /* = true */)
{
  std::map<std::string, JSONSchemaTypeDefinitionPtr> types;
  CJsonRpcMethodMap methods;
  std::map<std::string, CVariant> notifications;

  int clientPermissions = client->GetPermissionFlags();
  int transportCapabilities = transport->GetCapabilities();

  if (filterByName.size() > 0)
  {
    std::string name = filterByName;

    if (filterByType == "method")
    {
      StringUtils::ToLower(name);

      CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator = m_actionMap.find(name);
      if (methodIterator != m_actionMap.end() &&
         (clientPermissions & methodIterator->second.permission) == methodIterator->second.permission && ((transportCapabilities & methodIterator->second.transportneed) == methodIterator->second.transportneed || !filterByTransport))
        methods.add(methodIterator->second);
      else
        return InvalidParams;
    }
    else if (filterByType == "namespace")
    {
      // append a . delimiter to make sure we check for a namespace
      StringUtils::ToLower(name);
      name.append(".");

      CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
      CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = m_actionMap.end();
      for (methodIterator = m_actionMap.begin(); methodIterator != methodIteratorEnd; methodIterator++)
      {
        // Check if the given name is at the very beginning of the method name
        if (methodIterator->first.find(name) == 0 &&
           (clientPermissions & methodIterator->second.permission) == methodIterator->second.permission && ((transportCapabilities & methodIterator->second.transportneed) == methodIterator->second.transportneed || !filterByTransport))
          methods.add(methodIterator->second);
      }

      if (methods.begin() == methods.end())
        return InvalidParams;
    }
    else if (filterByType == "type")
    {
      std::map<std::string, JSONSchemaTypeDefinitionPtr>::const_iterator typeIterator = m_types.find(name);
      if (typeIterator != m_types.end())
        types[typeIterator->first] = typeIterator->second;
      else
        return InvalidParams;
    }
    else if (filterByType == "notification")
    {
      std::map<std::string, CVariant>::const_iterator notificationIterator = m_notifications.find(name);
      if (notificationIterator != m_notifications.end())
        notifications[notificationIterator->first] = notificationIterator->second;
      else
        return InvalidParams;
    }
    else
      return InvalidParams;

    // If we need to print all referenced types we have to go through all parameters etc
    if (printReferences)
    {
      std::vector<std::string> referencedTypes;

      // Loop through all printed types to get all referenced types
      std::map<std::string, JSONSchemaTypeDefinitionPtr>::const_iterator typeIterator;
      std::map<std::string, JSONSchemaTypeDefinitionPtr>::const_iterator typeIteratorEnd = types.end();
      for (typeIterator = types.begin(); typeIterator != typeIteratorEnd; ++typeIterator)
        getReferencedTypes(typeIterator->second, referencedTypes);

      // Loop through all printed method's parameters and return value to get all referenced types
      CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
      CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = methods.end();
      for (methodIterator = methods.begin(); methodIterator != methodIteratorEnd; methodIterator++)
      {
        for (unsigned int index = 0; index < methodIterator->second.parameters.size(); index++)
          getReferencedTypes(methodIterator->second.parameters.at(index), referencedTypes);

        getReferencedTypes(methodIterator->second.returns, referencedTypes);
      }

      for (unsigned int index = 0; index < referencedTypes.size(); index++)
      {
        std::map<std::string, JSONSchemaTypeDefinitionPtr>::const_iterator typeIterator = m_types.find(referencedTypes.at(index));
        if (typeIterator != m_types.end())
          types[typeIterator->first] = typeIterator->second;
      }
    }
  }
  else
  {
    types = m_types;
    methods = m_actionMap;
    notifications = m_notifications;
  }

  // Print the header
  result["id"] = JSONRPC_SERVICE_ID;
  result["version"] = JSONRPC_SERVICE_VERSION;
  result["description"] = JSONRPC_SERVICE_DESCRIPTION;

  std::map<std::string, JSONSchemaTypeDefinitionPtr>::const_iterator typeIterator;
  std::map<std::string, JSONSchemaTypeDefinitionPtr>::const_iterator typeIteratorEnd = types.end();
  for (typeIterator = types.begin(); typeIterator != typeIteratorEnd; ++typeIterator)
  {
    CVariant currentType = CVariant(CVariant::VariantTypeObject);
    typeIterator->second->Print(false, true, true, printDescriptions, currentType);

    result["types"][typeIterator->first] = currentType;
  }

  // Iterate through all json rpc methods
  CJsonRpcMethodMap::JsonRpcMethodIterator methodIterator;
  CJsonRpcMethodMap::JsonRpcMethodIterator methodIteratorEnd = methods.end();
  for (methodIterator = methods.begin(); methodIterator != methodIteratorEnd; methodIterator++)
  {
    if ((clientPermissions & methodIterator->second.permission) != methodIterator->second.permission || ((transportCapabilities & methodIterator->second.transportneed) != methodIterator->second.transportneed && filterByTransport))
      continue;

    CVariant currentMethod = CVariant(CVariant::VariantTypeObject);

    currentMethod["type"] = "method";
    if (printDescriptions && !methodIterator->second.description.empty())
      currentMethod["description"] = methodIterator->second.description;
    if (printMetadata)
    {
      CVariant permissions(CVariant::VariantTypeArray);
      for (int i = ReadData; i <= OPERATION_PERMISSION_ALL; i *= 2)
      {
        if ((methodIterator->second.permission & i) == i)
          permissions.push_back(PermissionToString((OperationPermission)i));
      }

      if (permissions.size() == 1)
        currentMethod["permission"] = permissions[0];
      else
        currentMethod["permission"] = permissions;
    }

    currentMethod["params"] = CVariant(CVariant::VariantTypeArray);
    for (unsigned int paramIndex = 0; paramIndex < methodIterator->second.parameters.size(); paramIndex++)
    {
      CVariant param = CVariant(CVariant::VariantTypeObject);
      methodIterator->second.parameters.at(paramIndex)->Print(true, false, true, printDescriptions, param);
      currentMethod["params"].append(param);
    }

    methodIterator->second.returns->Print(false, false, false, printDescriptions, currentMethod["returns"]);

    result["methods"][methodIterator->second.name] = currentMethod;
  }

  // Print notification description
  std::map<std::string, CVariant>::const_iterator notificationIterator;
  std::map<std::string, CVariant>::const_iterator notificationIteratorEnd = notifications.end();
  for (notificationIterator = notifications.begin(); notificationIterator != notificationIteratorEnd; ++notificationIterator)
    result["notifications"][notificationIterator->first] = notificationIterator->second[notificationIterator->first];

  return OK;
}

JSONRPC_STATUS CJSONServiceDescription::CheckCall(const char* const method, const CVariant &requestParameters, ITransportLayer *transport, IClient *client, bool notification, MethodCall &methodCall, CVariant &outputParameters)
{
  CJsonRpcMethodMap::JsonRpcMethodIterator iter = m_actionMap.find(method);
  if (iter != m_actionMap.end())
    return iter->second.Check(requestParameters, transport, client, notification, methodCall, outputParameters);

  return MethodNotFound;
}

JSONSchemaTypeDefinitionPtr CJSONServiceDescription::GetType(const std::string &identification)
{
  std::map<std::string, JSONSchemaTypeDefinitionPtr>::iterator iter = m_types.find(identification);
  if (iter == m_types.end())
    return JSONSchemaTypeDefinitionPtr();
  
  return iter->second;
}

bool CJSONServiceDescription::parseJSONSchemaType(const CVariant &value, std::vector<JSONSchemaTypeDefinitionPtr>& typeDefinitions, JSONSchemaType &schemaType, std::string &missingReference)
{
  missingReference.clear();
  schemaType = AnyValue;

  if (value.isArray())
  {
    int parsedType = 0;
    // If the defined type is an array, we have
    // to handle a union type
    for (unsigned int typeIndex = 0; typeIndex < value.size(); typeIndex++)
    {
      JSONSchemaTypeDefinitionPtr definition = JSONSchemaTypeDefinitionPtr(new JSONSchemaTypeDefinition());
      // If the type is a string try to parse it
      if (value[typeIndex].isString())
        definition->type = StringToSchemaValueType(value[typeIndex].asString());
      else if (value[typeIndex].isObject())
      {
        if (!definition->Parse(value[typeIndex]))
        {
          missingReference = definition->missingReference;
          CLog::Log(LOGERROR, "JSONRPC: Invalid type schema in union type definition");
          return false;
        }
      }
      else
      {
        CLog::Log(LOGWARNING, "JSONRPC: Invalid type in union type definition");
        return false;
      }

      definition->optional = false;
      typeDefinitions.push_back(definition);
      parsedType |= definition->type;
    }

    // If the type has not been set yet set it to "any"
    if (parsedType != 0)
      schemaType = (JSONSchemaType)parsedType;

    return true;
  }
  
  if (value.isString())
  {
    schemaType = StringToSchemaValueType(value.asString());
    return true;
  }
  
  return false;
}

void CJSONServiceDescription::addReferenceTypeDefinition(JSONSchemaTypeDefinitionPtr typeDefinition)
{
  // If the given json value is no object or does not contain an "id" field
  // of type string it is no valid type definition
  if (typeDefinition->ID.empty())
    return;

  // If the id has already been defined we ignore the type definition
  if (m_types.find(typeDefinition->ID) != m_types.end())
    return;

  // Add the type to the list of type definitions
  m_types[typeDefinition->ID] = typeDefinition;
  
  IncompleteSchemaDefinitionMap::iterator iter = m_incompleteDefinitions.find(typeDefinition->ID);
  if (iter == m_incompleteDefinitions.end())
    return;
    
  CLog::Log(LOGINFO, "JSONRPC: Resolving incomplete types/methods referencing %s", typeDefinition->ID.c_str());
  for (unsigned int index = 0; index < iter->second.size(); index++)
  {
    if (iter->second[index].Type == SchemaDefinitionType)
      AddType(iter->second[index].Schema);
    else
      AddMethod(iter->second[index].Schema, iter->second[index].Method);
  }
  
  m_incompleteDefinitions.erase(typeDefinition->ID);
}

void CJSONServiceDescription::removeReferenceTypeDefinition(const std::string &typeID)
{
  if (typeID.empty())
    return;

  std::map<std::string, JSONSchemaTypeDefinitionPtr>::iterator type = m_types.find(typeID);
  if (type != m_types.end())
    m_types.erase(type);
}

void CJSONServiceDescription::getReferencedTypes(const JSONSchemaTypeDefinitionPtr type, std::vector<std::string> &referencedTypes)
{
  // If the current type is a referenceable object, we can add it to the list
  if (type->ID.size() > 0)
  {
    for (unsigned int index = 0; index < referencedTypes.size(); index++)
    {
      // The referenceable object has already been added to the list so we can just skip it
      if (type->ID == referencedTypes.at(index))
        return;
    }

    referencedTypes.push_back(type->ID);
  }

  // If the current type is an object we need to check its properties
  if (HasType(type->type, ObjectValue))
  {
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator iter;
    JSONSchemaTypeDefinition::CJsonSchemaPropertiesMap::JSONSchemaPropertiesIterator iterEnd = type->properties.end();
    for (iter = type->properties.begin(); iter != iterEnd; ++iter)
      getReferencedTypes(iter->second, referencedTypes);
  }
  // If the current type is an array we need to check its items
  if (HasType(type->type, ArrayValue))
  {
    unsigned int index;
    for (index = 0; index < type->items.size(); index++)
      getReferencedTypes(type->items.at(index), referencedTypes);

    for (index = 0; index < type->additionalItems.size(); index++)
      getReferencedTypes(type->additionalItems.at(index), referencedTypes);
  }

  // If the current type extends others type we need to check those types
  for (unsigned int index = 0; index < type->extends.size(); index++)
    getReferencedTypes(type->extends.at(index), referencedTypes);

  // If the current type is a union type we need to check those types
  for (unsigned int index = 0; index < type->unionTypes.size(); index++)
    getReferencedTypes(type->unionTypes.at(index), referencedTypes);
}

CJSONServiceDescription::CJsonRpcMethodMap::CJsonRpcMethodMap():
  m_actionmap(std::map<std::string, JsonRpcMethod>())
{
}

void CJSONServiceDescription::CJsonRpcMethodMap::clear()
{
  m_actionmap.clear();
}

void CJSONServiceDescription::CJsonRpcMethodMap::add(const JsonRpcMethod &method)
{
  std::string name = method.name;
  StringUtils::ToLower(name);
  m_actionmap[name] = method;
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::begin() const
{
  return m_actionmap.begin();
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::find(const std::string& key) const
{
  return m_actionmap.find(key);
}

CJSONServiceDescription::CJsonRpcMethodMap::JsonRpcMethodIterator CJSONServiceDescription::CJsonRpcMethodMap::end() const
{
  return m_actionmap.end();
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
//./../../../tools/jsd_builder/jsd_builder.o  6.5.5  ezeesystem.txt methods.json types.json notifications.json
#include "PictureLibrary.h"
#include "pictures/PictureDatabase.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "pictures/tags/PictureInfoTag.h"
#include "pictures/Face.h"
#include "pictures/PictureAlbum.h"
#include "pictures/Picture.h"
#include "pictures/Face.h"
#include "ApplicationMessenger.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "pictures/infoscanner/PictureAlbumInfo.h"
#include "TextureCache.h"
#include "settings/MediaSourceSettings.h"

using namespace PICTURE_INFO;
using namespace JSONRPC;
using namespace XFILE;

JSONRPC_STATUS CPictureLibrary::GetFaces(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://faces/");
  int locationID = -1, albumID = -1, pictureID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isMember("albumid"))
    albumID = (int)filter["albumid"].asInteger();
  else if (filter.isMember("album"))
    pictureUrl.AddOption("album", filter["album"].asString());
  else if (filter.isMember("pictureid"))
    pictureID = (int)filter["pictureid"].asInteger();
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("faces", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  bool albumFacesOnly = !CSettings::Get().GetBool("picturelibrary.showcompilationfaces");
  if (parameterObject["albumfacesonly"].isBoolean())
    albumFacesOnly = parameterObject["albumfacesonly"].asBoolean();
  
  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;
  
  CFileItemList items;
  if (!picturedatabase.GetFacesNav(pictureUrl.ToString(), items, albumFacesOnly, locationID, albumID, pictureID, CDatabase::Filter(), sorting))
    return InternalError;
  
  // Add "face" to "properties" array by default
  CVariant param = parameterObject;
  if (!param.isMember("properties"))
    param["properties"] = CVariant(CVariant::VariantTypeArray);
  param["properties"].append("face");
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("faceid", false, "faces", items, param, result, size, false);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetFaceDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int faceID = (int)parameterObject["faceid"].asInteger();
  
  CPictureDbUrl pictureUrl;
  if (!pictureUrl.FromString("picturedb://faces/"))
    return InternalError;
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  pictureUrl.AddOption("faceid", faceID);
  
  CFileItemList items;
  CDatabase::Filter filter;
  if (!picturedatabase.GetFacesByWhere(pictureUrl.ToString(), filter, items) || items.Size() != 1)
    return InvalidParams;
  
  // Add "face" to "properties" array by default
  CVariant param = parameterObject;
  if (!param.isMember("properties"))
    param["properties"] = CVariant(CVariant::VariantTypeArray);
  param["properties"].append("face");
  
  HandleFileItem("faceid", false, "facedetails", items[0], param, param["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetPictureAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://albums/");
  int faceID = -1, locationID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("faceid"))
    faceID = (int)filter["faceid"].asInteger();
  else if (filter.isMember("face"))
    pictureUrl.AddOption("face", filter["face"].asString());
  else if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isMember("picturetype"))
    pictureUrl.AddOption("picturetype", filter["picturetype"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("albums", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;
  
  CFileItemList items;
  if (!picturedatabase.GetPictureAlbumsNav(pictureUrl.ToString(), items, locationID, faceID, CDatabase::Filter(), sorting))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("albumid", false, "picturealbums", items, parameterObject, result, size, false);
  
  return OK;
}


JSONRPC_STATUS CPictureLibrary::GetVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://albums/");
  int faceID = -1, locationID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("faceid"))
    faceID = (int)filter["faceid"].asInteger();
  else if (filter.isMember("face"))
    pictureUrl.AddOption("face", filter["face"].asString());
  else if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isMember("picturetype"))
    pictureUrl.AddOption("picturetype", filter["picturetype"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("albums", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;
  
  CFileItemList items;
  if (!picturedatabase.GetVideoAlbumsNav(pictureUrl.ToString(), items, locationID, faceID, CDatabase::Filter(), sorting))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("albumid", false, "picturealbums", items, parameterObject, result, size, false);
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::AddPictureAlbum(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CStdString strAlbum = parameterObject["title"].asString();
  CStdString strLocation = parameterObject["location"].asString();
  CStdString strFace = parameterObject["faces"].asString();
  CLog::Log(LOGINFO, "JSONRPC: Add album  '%s'\n", strAlbum.c_str());
  
  int idAlbum = picturedatabase.AddPictureAlbum(strAlbum, strLocation, strFace);
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://albums/");
  
  pictureUrl.AddOption("albumid", idAlbum);
  int faceID = -1, locationID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("faceid"))
    faceID = (int)filter["faceid"].asInteger();
  else if (filter.isMember("face"))
    pictureUrl.AddOption("face", filter["face"].asString());
  else if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("albums", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  CFileItemList items;
  if (!picturedatabase.GetPictureAlbumsNav(pictureUrl.ToString(), items, locationID, faceID, CDatabase::Filter()))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("albumid", false, "picturealbums", items, parameterObject, result, size, false);
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::AddVideoAlbum(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CStdString strAlbum = parameterObject["title"].asString();
  CStdString strLocation = parameterObject["location"].asString();
  CStdString strFace = parameterObject["faces"].asString();
  CLog::Log(LOGINFO, "JSONRPC: Add album  '%s'\n", strAlbum.c_str());
  
  int idAlbum = picturedatabase.AddPictureAlbum(strAlbum, strLocation, strFace);
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://albums/");
  
  pictureUrl.AddOption("albumid", idAlbum);
  int faceID = -1, locationID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("faceid"))
    faceID = (int)filter["faceid"].asInteger();
  else if (filter.isMember("face"))
    pictureUrl.AddOption("face", filter["face"].asString());
  else if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("albums", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  CFileItemList items;
  if (!picturedatabase.GetPictureAlbumsNav(pictureUrl.ToString(), items, locationID, faceID, CDatabase::Filter()))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("albumid", false, "picturealbums", items, parameterObject, result, size, false);
  
  return OK;
}


JSONRPC_STATUS CPictureLibrary::GetPictureAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int albumID = (int)parameterObject["albumid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureAlbum album;
  if (!picturedatabase.GetPictureAlbumInfo(albumID, album, NULL))
    return InvalidParams;
  
  CStdString path;
  if (!picturedatabase.GetPictureAlbumPath(albumID, path))
    return InternalError;
  
  CFileItemPtr albumItem;
  FillPictureAlbumItem(album, path, albumItem);
  
  CFileItemList items;
  items.Add(albumItem);
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItem("albumid", false, "albumdetails", items[0], parameterObject, parameterObject["properties"], result, false);
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetVideoAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int albumID = (int)parameterObject["albumid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureAlbum album;
  if (!picturedatabase.GetPictureAlbumInfo(albumID, album, NULL))
    return InvalidParams;
  
  CStdString path;
  if (!picturedatabase.GetPictureAlbumPath(albumID, path))
    return InternalError;
  
  CFileItemPtr albumItem;
  FillPictureAlbumItem(album, path, albumItem);
  
  CFileItemList items;
  items.Add(albumItem);
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItem("albumid", false, "albumdetails", items[0], parameterObject, parameterObject["properties"], result, false);
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::AddPicture(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CStdString strAlbum = parameterObject["album"].c_str();
  CStdString strTitle = parameterObject["title"].c_str();
  CStdString strComment = parameterObject["comment"].c_str();
  CStdString strLocation = parameterObject["location"].c_str();
  CStdString strTaken = parameterObject["takenon"].c_str();
  CStdString strType = parameterObject["picturetype"].c_str();
  CStdString strOrientation = parameterObject["orientation"].c_str();
  
  //take the first source as the path
  VECSOURCES *shares = CMediaSourceSettings::Get().GetSources("pictures");
  if( !shares )
    return InternalError;
  
  //create the thumbnail and pass it in
  CStdString strPicturePath = shares->at(0).strPath + strTitle;
  CStdString strFaces = parameterObject["faces"].c_str();
  int idAlbum = picturedatabase.GetPictureAlbumByName(strAlbum);
  
  if(idAlbum <= 0 )
  {
    idAlbum = picturedatabase.AddPictureAlbum(strAlbum, strFaces, strLocation);
    
    CPictureAlbum album;
    album.idAlbum = idAlbum;
    album.strLabel = strAlbum;
    album.thumbURL.m_xml = strPicturePath;
    CPicture pic;
    pic.strFileName = strPicturePath;
    
    picturedatabase.SetPictureAlbumInfo(album.idAlbum,
                                        album,
                                        album.pictures);
  }
  
  std::vector<std::string> vecLocations;
  vecLocations.push_back(strLocation);
  
  std::vector<std::string> vecFaces;
  vecFaces.push_back(strFaces);
  
  int idPicture = picturedatabase.AddPicture(idAlbum, strTitle, strOrientation, strPicturePath, strComment, strPicturePath, vecFaces, vecLocations, strTaken);
  
  if( idPicture <=0 )
    return InternalError;
  
  // set the thumbnail for photo
  CLog::Log(LOGINFO, "JSONRPC: Add Picture Thumb  '%s'\n", strPicturePath.c_str());
  CTextureCache::Get().BackgroundCacheImage(strPicturePath);
  picturedatabase.SetArtForItem(idPicture, "picture", "thumb", strPicturePath);
  
  //set the latest added photo as the thumbnail for the album
  CTextureCache::Get().BackgroundCacheImage(strPicturePath);
  picturedatabase.SetArtForItem(idAlbum, "album", "thumb", strPicturePath);
  CTextureCache::Get().BackgroundCacheImage(strPicturePath);
  picturedatabase.SetArtForItem(idAlbum, "album", "fanart", strPicturePath);
  
  
  return GetPictures(method, transport, client, parameterObject, result);
}

JSONRPC_STATUS CPictureLibrary::AddVideo(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CStdString strAlbum = parameterObject["album"].c_str();
  CStdString strTitle = parameterObject["title"].c_str();
  CStdString strThumb = parameterObject["thumbnail"].c_str();
  CStdString strComment = parameterObject["comment"].c_str();
  CStdString strLocation = parameterObject["location"].c_str();
  CStdString strTaken = parameterObject["takenon"].c_str();
  CStdString strType = parameterObject["picturetype"].c_str();
  CStdString strOrientation = parameterObject["orientation"].c_str();
  
  //take the first source as the path
  VECSOURCES *shares = CMediaSourceSettings::Get().GetSources("pictures");
  if( !shares )
    return InternalError;
  
  //create the thumbnail and pass it in
  CStdString strPicturePath = shares->at(0).strPath + strThumb;
  CStdString strFaces = parameterObject["faces"].c_str();
  int idAlbum = picturedatabase.GetVideoAlbumByName(strAlbum);
  
  if(idAlbum <= 0 )
  {
    idAlbum = picturedatabase.AddVideoAlbum(strAlbum, strFaces, strLocation);
    
    CPictureAlbum album;
    album.idAlbum = idAlbum;
    album.strLabel = strAlbum;
    album.thumbURL.m_xml = strPicturePath;
    CPicture pic;
    pic.strFileName = strPicturePath;
    
    picturedatabase.SetPictureAlbumInfo(album.idAlbum,
                                        album,
                                        album.pictures);
  }
  
  std::vector<std::string> vecLocations;
  vecLocations.push_back(strLocation);
  
  std::vector<std::string> vecFaces;
  vecFaces.push_back(strFaces);
  
  int idPicture = picturedatabase.AddVideo(idAlbum, strTitle, strOrientation, strPicturePath, strComment, strPicturePath, vecFaces, vecLocations, strTaken);
  
  if( idPicture <=0 )
    return InternalError;
  
  // set the thumbnail for photo
  CTextureCache::Get().BackgroundCacheImage(strPicturePath);
  picturedatabase.SetArtForItem(idPicture, "picture", "thumb", strPicturePath);
  
  //set the latest added photo as the thumbnail for the album
  CTextureCache::Get().BackgroundCacheImage(strPicturePath);
  picturedatabase.SetArtForItem(idAlbum, "album", "thumb", strPicturePath);
  CTextureCache::Get().BackgroundCacheImage(strPicturePath);
  picturedatabase.SetArtForItem(idAlbum, "album", "fanart", strPicturePath);
  
  
  return GetVideos(method, transport, client, parameterObject, result);
}

JSONRPC_STATUS CPictureLibrary::GetPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://pictures/");
  int locationID = -1, albumID = -1, faceID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("faceid"))
    faceID = (int)filter["faceid"].asInteger();
  else if (filter.isMember("face"))
    pictureUrl.AddOption("face", filter["face"].asString());
  else if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isMember("albumid"))
    albumID = (int)filter["albumid"].asInteger();
  else if (filter.isMember("album"))
    pictureUrl.AddOption("album", filter["album"].asString());
  else if (filter.isMember("picturetype"))
    pictureUrl.AddOption("picturetype", filter["picturetype"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("pictures", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;
  
  CFileItemList items;
  if (!picturedatabase.GetPicturesNav(pictureUrl.ToString(), items, locationID, faceID, albumID, sorting))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  
  
  // we make sure there is only one pictures path
  // or get the relationship between picture and path
  // set the thumbnail for the photo
  for( int i=0; i<items.Size(); i++){
    items.Get(i)->SetPath(items.Get(i)->GetPictureInfoTag()->GetURL());
  }
  
  HandleFileItemList("pictureid", true, "pictures", items, parameterObject, result, size, false);
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureDbUrl pictureUrl;
  pictureUrl.FromString("picturedb://pictures/");
  int locationID = -1, albumID = -1, faceID = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("faceid"))
    faceID = (int)filter["faceid"].asInteger();
  else if (filter.isMember("face"))
    pictureUrl.AddOption("face", filter["face"].asString());
  else if (filter.isMember("locationid"))
    locationID = (int)filter["locationid"].asInteger();
  else if (filter.isMember("location"))
    pictureUrl.AddOption("location", filter["location"].asString());
  else if (filter.isMember("albumid"))
    albumID = (int)filter["albumid"].asInteger();
  else if (filter.isMember("album"))
    pictureUrl.AddOption("album", filter["album"].asString());
  else if (filter.isMember("picturetype"))
    pictureUrl.AddOption("picturetype", filter["picturetype"].asString());
  else if (filter.isObject())
  {
    CStdString xsp;
    if (!GetXspFiltering("pictures", filter, xsp))
      return InvalidParams;
    
    pictureUrl.AddOption("xsp", xsp);
  }
  
  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;
  
  CFileItemList items;
  if (!picturedatabase.GetVideosNav(pictureUrl.ToString(), items, locationID, faceID, albumID, sorting))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  
  
  // we make sure there is only one pictures path
  // or get the relationship between picture and path
  // set the thumbnail for the photo
  for( int i=0; i<items.Size(); i++){
    items.Get(i)->SetPath(items.Get(i)->GetPictureInfoTag()->GetURL());
  }
  
  HandleFileItemList("pictureid", true, "pictures", items, parameterObject, result, size, false);
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetPictureDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int idPicture = (int)parameterObject["pictureid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPicture picture;
  if (!picturedatabase.GetPicture(idPicture, picture))
    return InvalidParams;
  
  CFileItemList items;
  //items.Add(CFileItemPtr(new CFileItem(picture)));
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItem("pictureid", false, "picturedetails", items[0], parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyAddedPictureAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  VECPICTUREALBUMS albums;
  if (!picturedatabase.GetRecentlyAddedPictureAlbums(albums))
    return InternalError;
  
  CFileItemList items;
  for (unsigned int index = 0; index < albums.size(); index++)
  {
    CStdString path;
    path.Format("picturedb://recentlyaddedalbums/%i/", albums[index].idAlbum);
    
    CFileItemPtr item;
    FillPictureAlbumItem(albums[index], path, item);
    items.Add(item);
  }
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyAddedPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  int amount = (int)parameterObject["albumlimit"].asInteger();
  if (amount < 0)
    amount = 0;
  
  CFileItemList items;
  if (!picturedatabase.GetRecentlyAddedPictureAlbumPictures("picturedb://", items, (unsigned int)amount))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("pictureid", true, "pictures", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyPlayedPictureAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  VECPICTUREALBUMS albums;
  if (!picturedatabase.GetRecentlyPlayedPictureAlbums(albums))
    return InternalError;
  
  CFileItemList items;
  for (unsigned int index = 0; index < albums.size(); index++)
  {
    CStdString path;
    path.Format("picturedb://recentlyplayedalbums/%i/", albums[index].idAlbum);
    
    CFileItemPtr item;
    FillPictureAlbumItem(albums[index], path, item);
    items.Add(item);
  }
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyPlayedPictures(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CFileItemList items;
  if (!picturedatabase.GetRecentlyPlayedPictureAlbumPictures("picturedb://", items))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("pictureid", true, "pictures", items, parameterObject, result);
  return OK;
}


JSONRPC_STATUS CPictureLibrary::GetVideoDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int idPicture = (int)parameterObject["pictureid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPicture picture;
  if (!picturedatabase.GetPicture(idPicture, picture))
    return InvalidParams;
  
  CFileItemList items;
  //items.Add(CFileItemPtr(new CFileItem(picture)));
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItem("pictureid", false, "picturedetails", items[0], parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyAddedVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  VECPICTUREALBUMS albums;
  if (!picturedatabase.GetRecentlyAddedPictureAlbums(albums))
    return InternalError;
  
  CFileItemList items;
  for (unsigned int index = 0; index < albums.size(); index++)
  {
    CStdString path;
    path.Format("picturedb://recentlyaddedalbums/%i/", albums[index].idAlbum);
    
    CFileItemPtr item;
    FillPictureAlbumItem(albums[index], path, item);
    items.Add(item);
  }
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyAddedVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  int amount = (int)parameterObject["albumlimit"].asInteger();
  if (amount < 0)
    amount = 0;
  
  CFileItemList items;
  if (!picturedatabase.GetRecentlyAddedPictureAlbumPictures("picturedb://", items, (unsigned int)amount))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("pictureid", true, "pictures", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyPlayedVideoAlbums(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  VECPICTUREALBUMS albums;
  if (!picturedatabase.GetRecentlyPlayedPictureAlbums(albums))
    return InternalError;
  
  CFileItemList items;
  for (unsigned int index = 0; index < albums.size(); index++)
  {
    CStdString path;
    path.Format("picturedb://recentlyplayedalbums/%i/", albums[index].idAlbum);
    
    CFileItemPtr item;
    FillPictureAlbumItem(albums[index], path, item);
    items.Add(item);
  }
  
  JSONRPC_STATUS ret = GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("albumid", false, "albums", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetRecentlyPlayedVideos(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CFileItemList items;
  if (!picturedatabase.GetRecentlyPlayedPictureAlbumPictures("picturedb://", items))
    return InternalError;
  
  JSONRPC_STATUS ret = GetAdditionalPictureDetails(parameterObject, items, picturedatabase);
  if (ret != OK)
    return ret;
  
  HandleFileItemList("pictureid", true, "pictures", items, parameterObject, result);
  return OK;
}
JSONRPC_STATUS CPictureLibrary::GetLocations(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CFileItemList items;
  if (!picturedatabase.GetLocationsNav("picturedb://locations/", items))
    return InternalError;
  
  /* need to set strTitle in each item*/
  for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    items[i]->GetPictureInfoTag()->SetTitle(items[i]->GetLabel());
  
  HandleFileItemList("locationid", false, "locations", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CPictureLibrary::SetFaceDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["faceid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CFace face;
  if (!picturedatabase.GetFaceInfo(id, face) || face.idFace <= 0)
    return InvalidParams;
  
  if (ParameterNotNull(parameterObject, "face"))
    face.strFace = parameterObject["face"].asString();
  if (ParameterNotNull(parameterObject, "instrument"))
    CopyStringArray(parameterObject["instrument"], face.instruments);
  if (ParameterNotNull(parameterObject, "style"))
    CopyStringArray(parameterObject["style"], face.styles);
  if (ParameterNotNull(parameterObject, "mood"))
    CopyStringArray(parameterObject["mood"], face.moods);
  if (ParameterNotNull(parameterObject, "born"))
    face.strBorn = parameterObject["born"].asString();
  if (ParameterNotNull(parameterObject, "formed"))
    face.strFormed = parameterObject["formed"].asString();
  if (ParameterNotNull(parameterObject, "description"))
    face.strBiography = parameterObject["description"].asString();
  if (ParameterNotNull(parameterObject, "location"))
    CopyStringArray(parameterObject["location"], face.location);
  if (ParameterNotNull(parameterObject, "died"))
    face.strDied = parameterObject["died"].asString();
  if (ParameterNotNull(parameterObject, "disbanded"))
    face.strDisbanded = parameterObject["disbanded"].asString();
  if (ParameterNotNull(parameterObject, "yearsactive"))
    CopyStringArray(parameterObject["yearsactive"], face.yearsActive);
  
  if (picturedatabase.SetFaceInfo(id, face) <= 0)
    return InternalError;
  
  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CPictureLibrary::SetPictureAlbumDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["albumid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPictureAlbum album;
  VECPICTURES pictures;
  if (!picturedatabase.GetPictureAlbumInfo(id, album, &pictures) || album.idAlbum <= 0)
    return InvalidParams;
  
  if (ParameterNotNull(parameterObject, "title"))
    album.strAlbum = parameterObject["title"].asString();
  if (ParameterNotNull(parameterObject, "face"))
    CopyStringArray(parameterObject["face"], album.face);
  if (ParameterNotNull(parameterObject, "description"))
    album.strReview = parameterObject["description"].asString();
  if (ParameterNotNull(parameterObject, "location"))
    CopyStringArray(parameterObject["location"], album.location);
  if (ParameterNotNull(parameterObject, "theme"))
    CopyStringArray(parameterObject["theme"], album.themes);
  if (ParameterNotNull(parameterObject, "mood"))
    CopyStringArray(parameterObject["mood"], album.moods);
  if (ParameterNotNull(parameterObject, "style"))
    CopyStringArray(parameterObject["style"], album.styles);
  if (ParameterNotNull(parameterObject, "type"))
    album.strPictureType = parameterObject["picturetype"].asString();
  if (ParameterNotNull(parameterObject, "albumlabel"))
    album.strLabel = parameterObject["albumlabel"].asString();
  
  if (picturedatabase.SetPictureAlbumInfo(id, album, pictures) <= 0)
    return InternalError;
  
  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CPictureLibrary::SetPictureDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["pictureid"].asInteger();
  
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return InternalError;
  
  CPicture picture;
  if (!picturedatabase.GetPicture(id, picture) || picture.idPicture != id)
    return InvalidParams;
  
  if (ParameterNotNull(parameterObject, "title"))
    picture.strTitle = parameterObject["title"].asString();
  if (ParameterNotNull(parameterObject, "face"))
    CopyStringArray(parameterObject["face"], picture.face);
  if (ParameterNotNull(parameterObject, "albumface"))
    CopyStringArray(parameterObject["albumface"], picture.albumFace);
  if (ParameterNotNull(parameterObject, "location"))
    CopyStringArray(parameterObject["location"], picture.location);
  if (ParameterNotNull(parameterObject, "album"))
    picture.strAlbum = parameterObject["album"].asString();
  if (ParameterNotNull(parameterObject, "comment"))
    picture.strComment = parameterObject["comment"].asString();
  
  if (picturedatabase.UpdatePicture(id, picture.strTitle, picture.strFileName, picture.strComment, picture.strThumb, picture.face, picture.location,  picture.takenOn.GetAsDBDate().c_str()) <= 0)
    return InternalError;
  
  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CPictureLibrary::Scan(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string directory = parameterObject["directory"].asString();
  CStdString cmd;
  if (directory.empty())
    cmd = "updatelibrary(picture)";
  else
    cmd.Format("updatelibrary(picture, %s)", StringUtils::Paramify(directory).c_str());
  
  CApplicationMessenger::Get().ExecBuiltIn(cmd);
  return ACK;
}

JSONRPC_STATUS CPictureLibrary::Export(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString cmd;
  if (parameterObject["options"].isMember("path"))
    cmd.Format("exportlibrary(picture, false, %s)", StringUtils::Paramify(parameterObject["options"]["path"].asString()));
  else
    cmd.Format("exportlibrary(picture, true, %s, %s)",
               parameterObject["options"]["images"].asBoolean() ? "true" : "false",
               parameterObject["options"]["overwrite"].asBoolean() ? "true" : "false");
  
  CApplicationMessenger::Get().ExecBuiltIn(cmd);
  return ACK;
}

JSONRPC_STATUS CPictureLibrary::Clean(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CApplicationMessenger::Get().ExecBuiltIn("cleanlibrary(picture)");
  return ACK;
}

bool CPictureLibrary::FillFileItem(const CStdString &strFilename, CFileItemPtr &item, const CVariant &parameterObject /* = CVariant(CVariant::VariantTypeArray) */)
{
  CPictureDatabase picturedatabase;
  if (strFilename.empty() || !picturedatabase.Open())
    return false;
  
  if (CDirectory::Exists(strFilename))
  {
    CPictureAlbum album;
    int albumid = picturedatabase.GetPictureAlbumIdByPath(strFilename);
    if (!picturedatabase.GetPictureAlbumInfo(albumid, album, NULL))
      return false;
    
    item->SetFromPictureAlbum(album);
    
    CFileItemList items;
    items.Add(item);
    if (GetAdditionalPictureAlbumDetails(parameterObject, items, picturedatabase) != OK)
      return false;
  }
  else
  {
    CPicture picture;
    if (!picturedatabase.GetPictureByFileName(strFilename, picture))
      return false;
    
    item->SetFromPicture(picture);
    
    CFileItemList items;
    items.Add(item);
    if (GetAdditionalPictureDetails(parameterObject, items, picturedatabase) != OK)
      return false;
  }
  
  if (item->GetLabel().empty())
    item->SetLabel(CUtil::GetTitleFromPath(strFilename, false));
  if (item->GetLabel())
    item->SetLabel(URIUtils::GetFileName(strFilename));
  
  return true;
}

bool CPictureLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CPictureDatabase picturedatabase;
  if (!picturedatabase.Open())
    return false;
  
  CStdString file = parameterObject["file"].asString();
  int faceID = (int)parameterObject["faceid"].asInteger(-1);
  int albumID = (int)parameterObject["albumid"].asInteger(-1);
  int locationID = (int)parameterObject["locationid"].asInteger(-1);
  
  bool success = false;
  CFileItemPtr fileItem(new CFileItem());
  if (FillFileItem(file, fileItem, parameterObject))
  {
    success = true;
    list.Add(fileItem);
  }
  
  if (faceID != -1 || albumID != -1 || locationID != -1)
    success |= picturedatabase.GetPicturesNav("picturedb://pictures/", list, locationID, faceID, albumID);
  
  int pictureID = (int)parameterObject["pictureid"].asInteger(-1);
  if (pictureID != -1)
  {
    CPicture picture;
    if (picturedatabase.GetPicture(pictureID, picture))
    {
      list.Add(CFileItemPtr(new CFileItem(picture)));
      success = true;
    }
  }
  
  if (success)
  {
    // If we retrieved the list of pictures by "faceid"
    // we sort by album (and implicitly by track number)
    if (faceID != -1)
      list.Sort(SORT_METHOD_ALBUM_IGNORE_THE, SortOrderAscending);
    // If we retrieve the list of pictures by "locationid"
    // we sort by face (and implicitly by album and track number)
    else if (locationID != -1)
      list.Sort(SORT_METHOD_ARTIST_IGNORE_THE, SortOrderAscending);
    // otherwise we sort by track number
    else
      list.Sort(SORT_METHOD_TRACKNUM, SortOrderAscending);
    
  }
  
  return success;
}

void CPictureLibrary::FillPictureAlbumItem(const CPictureAlbum &album, const CStdString &path, CFileItemPtr &item)
{
  item = CFileItemPtr(new CFileItem(path, album));
}

JSONRPC_STATUS CPictureLibrary::GetAdditionalPictureAlbumDetails(const CVariant &parameterObject, CFileItemList &items, CPictureDatabase &picturedatabase)
{
  if (!picturedatabase.Open())
    return InternalError;
  
  std::set<std::string> checkProperties;
  checkProperties.insert("locationid");
  checkProperties.insert("faceid");
  std::set<std::string> additionalProperties;
  if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    return OK;
  
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    if (additionalProperties.find("locationid") != additionalProperties.end())
    {
      std::vector<int> locationids;
      if (picturedatabase.GetLocationsByPictureAlbum(item->GetPictureInfoTag()->GetDatabaseId(), locationids))
      {
        CVariant locationidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator locationid = locationids.begin(); locationid != locationids.end(); ++locationid)
          locationidObj.push_back(*locationid);
        
        item->SetProperty("locationid", locationidObj);
      }
    }
    if (additionalProperties.find("faceid") != additionalProperties.end())
    {
      std::vector<int> faceids;
      if (picturedatabase.GetFacesByPictureAlbum(item->GetPictureInfoTag()->GetDatabaseId(), true, faceids))
      {
        CVariant faceidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator faceid = faceids.begin(); faceid != faceids.end(); ++faceid)
          faceidObj.push_back(*faceid);
        
        item->SetProperty("faceid", faceidObj);
      }
    }
  }
  
  return OK;
}

JSONRPC_STATUS CPictureLibrary::GetAdditionalPictureDetails(const CVariant &parameterObject, CFileItemList &items, CPictureDatabase &picturedatabase)
{
  if (!picturedatabase.Open())
    return InternalError;
  
  std::set<std::string> checkProperties;
  checkProperties.insert("locationid");
  checkProperties.insert("faceid");
  checkProperties.insert("albumfaceid");
  std::set<std::string> additionalProperties;
  if (!CheckForAdditionalProperties(parameterObject["properties"], checkProperties, additionalProperties))
    return OK;
  
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    if (additionalProperties.find("locationid") != additionalProperties.end())
    {
      std::vector<int> locationids;
      if (picturedatabase.GetLocationsByPicture(item->GetPictureInfoTag()->GetDatabaseId(), locationids))
      {
        CVariant locationidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator locationid = locationids.begin(); locationid != locationids.end(); ++locationid)
          locationidObj.push_back(*locationid);
        
        item->SetProperty("locationid", locationidObj);
      }
    }
    if (additionalProperties.find("faceid") != additionalProperties.end())
    {
      std::vector<int> faceids;
      if (picturedatabase.GetFacesByPicture(item->GetPictureInfoTag()->GetDatabaseId(), true, faceids))
      {
        CVariant faceidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator faceid = faceids.begin(); faceid != faceids.end(); ++faceid)
          faceidObj.push_back(*faceid);
        
        item->SetProperty("faceid", faceidObj);
      }
    }
    if (additionalProperties.find("albumfaceid") != additionalProperties.end() && item->GetPictureInfoTag()->GetPictureAlbumId() > 0)
    {
      std::vector<int> albumfaceids;
      if (picturedatabase.GetFacesByPictureAlbum(item->GetPictureInfoTag()->GetPictureAlbumId(), true, albumfaceids))
      {
        CVariant albumfaceidObj(CVariant::VariantTypeArray);
        for (std::vector<int>::const_iterator albumfaceid = albumfaceids.begin(); albumfaceid != albumfaceids.end(); ++albumfaceid)
          albumfaceidObj.push_back(*albumfaceid);
        
        item->SetProperty("albumfaceid", albumfaceidObj);
      }
    }
  }
  
  return OK;
}

bool CPictureLibrary::CheckForAdditionalProperties(const CVariant &properties, const std::set<std::string> &checkProperties, std::set<std::string> &foundProperties)
{
  if (!properties.isArray() || properties.empty())
    return false;
  
  std::set<std::string> checkingProperties = checkProperties;
  for (CVariant::const_iterator_array itr = properties.begin_array(); itr != properties.end_array() && !checkingProperties.empty(); itr++)
  {
    if (!itr->isString())
      continue;
    
    std::string property = itr->asString();
    if (checkingProperties.find(property) != checkingProperties.end())
    {
      checkingProperties.erase(property);
      foundProperties.insert(property);
    }
  }
  
  return !foundProperties.empty();
}

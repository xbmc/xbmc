#pragma once
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

#include "utils/ISerializable.h"
#include "utils/ISortable.h"
#include "utils/Archive.h"
#include "../DllLibExif.h"
#include "XBDateTime.h"

#define SLIDE_FILE_NAME             900         // Note that not all image tags will be present for each image
#define SLIDE_FILE_PATH             901
#define SLIDE_FILE_SIZE             902
#define SLIDE_FILE_DATE             903
#define SLIDE_INDEX                 904
#define SLIDE_RESOLUTION            905
#define SLIDE_COMMENT               906
#define SLIDE_COLOUR                907
#define SLIDE_PROCESS               908

#define SLIDE_EXIF_LONG_DATE        917
#define SLIDE_EXIF_LONG_DATE_TIME   918
#define SLIDE_EXIF_DATE             919 /* Implementation only to just get
                                           localized date */
#define SLIDE_EXIF_DATE_TIME        920
#define SLIDE_EXIF_DESCRIPTION      921
#define SLIDE_EXIF_CAMERA_MAKE      922
#define SLIDE_EXIF_CAMERA_MODEL     923
#define SLIDE_EXIF_COMMENT          924
#define SLIDE_EXIF_SOFTWARE         925
#define SLIDE_EXIF_APERTURE         926
#define SLIDE_EXIF_FOCAL_LENGTH     927
#define SLIDE_EXIF_FOCUS_DIST       928
#define SLIDE_EXIF_EXPOSURE         929
#define SLIDE_EXIF_EXPOSURE_TIME    930
#define SLIDE_EXIF_EXPOSURE_BIAS    931
#define SLIDE_EXIF_EXPOSURE_MODE    932
#define SLIDE_EXIF_FLASH_USED       933
#define SLIDE_EXIF_WHITE_BALANCE    934
#define SLIDE_EXIF_LIGHT_SOURCE     935
#define SLIDE_EXIF_METERING_MODE    936
#define SLIDE_EXIF_ISO_EQUIV        937
#define SLIDE_EXIF_DIGITAL_ZOOM     938
#define SLIDE_EXIF_CCD_WIDTH        939
#define SLIDE_EXIF_GPS_LATITUDE     940
#define SLIDE_EXIF_GPS_LONGITUDE    941
#define SLIDE_EXIF_GPS_ALTITUDE     942
#define SLIDE_EXIF_ORIENTATION      943

#define SLIDE_IPTC_SUBLOCATION      957
#define SLIDE_IPTC_IMAGETYPE        958
#define SLIDE_IPTC_TIMECREATED      959
#define SLIDE_IPTC_SUP_CATEGORIES   960
#define SLIDE_IPTC_KEYWORDS         961
#define SLIDE_IPTC_CAPTION          962
#define SLIDE_IPTC_AUTHOR           963
#define SLIDE_IPTC_HEADLINE         964
#define SLIDE_IPTC_SPEC_INSTR       965
#define SLIDE_IPTC_CATEGORY         966
#define SLIDE_IPTC_BYLINE           967
#define SLIDE_IPTC_BYLINE_TITLE     968
#define SLIDE_IPTC_CREDIT           969
#define SLIDE_IPTC_SOURCE           970
#define SLIDE_IPTC_COPYRIGHT_NOTICE 971
#define SLIDE_IPTC_OBJECT_NAME      972
#define SLIDE_IPTC_CITY             973
#define SLIDE_IPTC_STATE            974
#define SLIDE_IPTC_COUNTRY          975
#define SLIDE_IPTC_TX_REFERENCE     976
#define SLIDE_IPTC_DATE             977
#define SLIDE_IPTC_URGENCY          978
#define SLIDE_IPTC_COUNTRY_CODE     979
#define SLIDE_IPTC_REF_SERVICE      980

class CFace;
class CPictureAlbum;
class CPicture;

namespace PICTURE_INFO
{
    class EmbeddedArtInfo
    {
    public:
        EmbeddedArtInfo() {};
        EmbeddedArtInfo(size_t size, const std::string &mime);
        void set(size_t size, const std::string &mime);
        void clear();
        bool empty() const;
        bool matches(const EmbeddedArtInfo &right) const;
        size_t      size;
        std::string mime;
    };
    
    class EmbeddedArt : public EmbeddedArtInfo
    {
    public:
        EmbeddedArt() {};
        EmbeddedArt(const uint8_t *data, size_t size, const std::string &mime);
        void set(const uint8_t *data, size_t size, const std::string &mime);
        std::vector<uint8_t> data;
    };

class CPictureInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
    CPictureInfoTag(void);
    CPictureInfoTag(const CPictureInfoTag& tag);
    virtual ~CPictureInfoTag();
    
  void Reset();    
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual void ToSortable(SortItem& sortable);
  const CPictureInfoTag& operator=(const CPictureInfoTag& item);
  const CStdString GetInfo(int info) const;

  bool Load(const CStdString &path);

  static int TranslateString(const CStdString &info);

  void SetInfo(int info, const CStdString& value);

  /**
   * GetDateTimeTaken() -- Returns the EXIF DateTimeOriginal for current picture
   * 
   * The exif library returns DateTimeOriginal if available else the other
   * DateTime tags. See libexif CExifParse::ProcessDir for details.
   */
  const CDateTime& GetDateTimeTaken() const;
private:
  void GetStringFromArchive(CArchive &ar, char *string, size_t length);
  ExifInfo_t m_exifInfo;
  IPTCInfo_t m_iptcInfo;
  bool       m_isInfoSetExternally;  // Set to true if metadata has been set by an external call to SetInfo
  CDateTime  m_dateTimeTaken;
  void ConvertDateTime();

    //////////////////////////////
    //support for JSON response
public:
    bool operator !=(const CPictureInfoTag& tag) const;
    bool Loaded() const;
    const CStdString& GetTitle() const;
    const CStdString& GetURL() const;
    const std::vector<std::string>& GetFace() const;
    const CStdString& GetPictureAlbum() const;
    int GetPictureAlbumId() const;
    const std::vector<std::string>& GetPictureAlbumFace() const;
    const std::vector<std::string>& GetLocation() const;
    int GetDatabaseId() const;
    const std::string &GetType() const;
    
    void GetReleaseDate(SYSTEMTIME& dateTime) const;
    CStdString GetYearString() const;
    const CStdString& GetComment() const;
    const CStdString& GetLyrics() const;
    const CDateTime& GetTakenOn() const;
  const CStdString &GetPictureType() const;
  const CStdString &GetOrientation() const;
    int  GetListeners() const;
    const EmbeddedArtInfo &GetCoverArtInfo() const;
    
    void SetURL(const CStdString& strURL);
    void SetTitle(const CStdString& strTitle);
    void SetFace(const CStdString& strFace);
    void SetFace(const std::vector<std::string>& faces);
    void SetAlbum(const CStdString& strAlbum);
    void SetAlbumId(const int iAlbumId);
    void SetAlbumFace(const CStdString& strAlbumFace);
    void SetAlbumFace(const std::vector<std::string>& albumFaces);
    void SetLocation(const CStdString& strLocation);
    void SetLocation(const std::vector<std::string>& locations);
    void SetDatabaseId(long id, const std::string &type);
    void SetPartOfSet(int m_iPartOfSet);
    void SetLoaded(bool bOnOff = true);
    void SetFace(const CFace& face);
    void SetAlbum(const CPictureAlbum& album);
    void SetPicture(const CPicture& picture);
    void SetComment(const CStdString& comment);
  void SetPictureType(const CStdString& type);
  void SetOrientation(const CStdString& orientation);
    void SetListeners(int listeners);
    void SetTakenOn(const CStdString& strTakenOn);
    void SetTakenOn(const CDateTime& strTakenOn);
    void SetCoverArtInfo(size_t size, const std::string &mimeType);
  
    /*! \brief Append a unique face to the face list
     Checks if we have this face already added, and if not adds it to the pictures face list.
     \param value face to add.
     */
    void AppendFace(const CStdString &face);
    
    /*! \brief Append a unique album face to the face list
     Checks if we have this album face already added, and if not adds it to the pictures album face list.
     \param albumFace album face to add.
     */
    void AppendAlbumFace(const CStdString &albumFace);
    
    /*! \brief Append a unique location to the location list
     Checks if we have this location already added, and if not adds it to the pictures location list.
     \param location location to add.
     */
    void AppendLocation(const CStdString &location);
    
    
    void Clear();
protected:
    /*! \brief Trim whitespace off the given string
     \param value string to trim
     \return trimmed value, with spaces removed from left and right, as well as carriage returns from the right.
     */
    CStdString Trim(const CStdString &value) const;
    
    CStdString m_strURL;
    CStdString m_strTitle;
    std::vector<std::string> m_face;
    CStdString m_strAlbum;
    std::vector<std::string> m_albumFace;
    std::vector<std::string> m_location;
  CStdString m_strOrientation;
  CStdString m_strType;
    CStdString m_strComment;
    CStdString m_strLyrics;
    CDateTime m_takenOn;
    int m_iDbId;
    std::string m_type; ///< item type "picture", "album", "face"
    bool m_bLoaded;
    int m_listeners;
    int m_iAlbumId;
  
    EmbeddedArtInfo m_coverArt; ///< art information
};
}

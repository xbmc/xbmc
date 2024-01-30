/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Mime.h"

#include "FileItem.h"
#include "URIUtils.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"

#include <algorithm>

const std::map<std::string, std::string> CMime::m_mimetypes = {
    {{"3dm", "x-world/x-3dmf"},
     {"3dmf", "x-world/x-3dmf"},
     {"3fr", "image/3fr"},
     {"a", "application/octet-stream"},
     {"aab", "application/x-authorware-bin"},
     {"aam", "application/x-authorware-map"},
     {"aas", "application/x-authorware-seg"},
     {"abc", "text/vnd.abc"},
     {"acgi", "text/html"},
     {"afl", "video/animaflex"},
     {"ai", "application/postscript"},
     {"aif", "audio/aiff"},
     {"aifc", "audio/x-aiff"},
     {"aiff", "audio/aiff"},
     {"aim", "application/x-aim"},
     {"aip", "text/x-audiosoft-intra"},
     {"ani", "application/x-navi-animation"},
     {"aos", "application/x-nokia-9000-communicator-add-on-software"},
     {"apng", "image/apng"},
     {"aps", "application/mime"},
     {"arc", "application/octet-stream"},
     {"arj", "application/arj"},
     {"art", "image/x-jg"},
     {"arw", "image/arw"},
     {"asf", "video/x-ms-asf"},
     {"asm", "text/x-asm"},
     {"asp", "text/asp"},
     {"asx", "video/x-ms-asf"},
     {"au", "audio/basic"},
     {"avi", "video/avi"},
     {"avs", "video/avs-video"},
     {"bcpio", "application/x-bcpio"},
     {"bin", "application/octet-stream"},
     {"bm", "image/bmp"},
     {"bmp", "image/bmp"},
     {"boo", "application/book"},
     {"book", "application/book"},
     {"boz", "application/x-bzip2"},
     {"bsh", "application/x-bsh"},
     {"bz", "application/x-bzip"},
     {"bz2", "application/x-bzip2"},
     {"c", "text/plain"},
     {"c++", "text/plain"},
     {"cat", "application/vnd.ms-pki.seccat"},
     {"cc", "text/plain"},
     {"ccad", "application/clariscad"},
     {"cco", "application/x-cocoa"},
     {"cdf", "application/cdf"},
     {"cer", "application/pkix-cert"},
     {"cer", "application/x-x509-ca-cert"},
     {"cha", "application/x-chat"},
     {"chat", "application/x-chat"},
     {"class", "application/java"},
     {"com", "application/octet-stream"},
     {"conf", "text/plain"},
     {"cpio", "application/x-cpio"},
     {"cpp", "text/x-c"},
     {"cpt", "application/x-cpt"},
     {"crl", "application/pkcs-crl"},
     {"crt", "application/pkix-cert"},
     {"cr2", "image/cr2"},
     {"crw", "image/crw"},
     {"csh", "application/x-csh"},
     {"css", "text/css"},
     {"cxx", "text/plain"},
     {"dcr", "application/x-director"},
     {"deepv", "application/x-deepv"},
     {"def", "text/plain"},
     {"der", "application/x-x509-ca-cert"},
     {"dif", "video/x-dv"},
     {"dir", "application/x-director"},
     {"dl", "video/dl"},
     {"divx", "video/x-msvideo"},
     {"dng", "image/dng"},
     {"doc", "application/msword"},
     {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
     {"dot", "application/msword"},
     {"dp", "application/commonground"},
     {"drw", "application/drafting"},
     {"dump", "application/octet-stream"},
     {"dv", "video/x-dv"},
     {"dvi", "application/x-dvi"},
     {"dwf", "model/vnd.dwf"},
     {"dwg", "image/vnd.dwg"},
     {"dxf", "image/vnd.dwg"},
     {"dxr", "application/x-director"},
     {"el", "text/x-script.elisp"},
     {"elc", "application/x-elc"},
     {"env", "application/x-envoy"},
     {"eps", "application/postscript"},
     {"erf", "image/erf"},
     {"es", "application/x-esrehber"},
     {"etx", "text/x-setext"},
     {"evy", "application/envoy"},
     {"exe", "application/octet-stream"},
     {"f", "text/x-fortran"},
     {"f77", "text/x-fortran"},
     {"f90", "text/x-fortran"},
     {"fdf", "application/vnd.fdf"},
     {"fif", "image/fif"},
     {"flac", "audio/flac"},
     {"fli", "video/fli"},
     {"flo", "image/florian"},
     {"flv", "video/x-flv"},
     {"flx", "text/vnd.fmi.flexstor"},
     {"fmf", "video/x-atomic3d-feature"},
     {"for", "text/plain"},
     {"for", "text/x-fortran"},
     {"fpx", "image/vnd.fpx"},
     {"frl", "application/freeloader"},
     {"funk", "audio/make"},
     {"g", "text/plain"},
     {"g3", "image/g3fax"},
     {"gif", "image/gif"},
     {"gl", "video/x-gl"},
     {"gsd", "audio/x-gsm"},
     {"gsm", "audio/x-gsm"},
     {"gsp", "application/x-gsp"},
     {"gss", "application/x-gss"},
     {"gtar", "application/x-gtar"},
     {"gz", "application/x-compressed"},
     {"gzip", "application/x-gzip"},
     {"h", "text/plain"},
     {"hdf", "application/x-hdf"},
     {"heic", "image/heic"},
     {"heif", "image/heif"},
     {"help", "application/x-helpfile"},
     {"hgl", "application/vnd.hp-hpgl"},
     {"hh", "text/plain"},
     {"hlb", "text/x-script"},
     {"hlp", "application/hlp"},
     {"hpg", "application/vnd.hp-hpgl"},
     {"hpgl", "application/vnd.hp-hpgl"},
     {"hqx", "application/binhex"},
     {"hta", "application/hta"},
     {"htc", "text/x-component"},
     {"htm", "text/html"},
     {"html", "text/html"},
     {"htmls", "text/html"},
     {"htt", "text/webviewhtml"},
     {"htx", "text/html"},
     {"ice", "x-conference/x-cooltalk"},
     {"ico", "image/x-icon"},
     {"idc", "text/plain"},
     {"ief", "image/ief"},
     {"iefs", "image/ief"},
     {"iges", "application/iges"},
     {"igs", "application/iges"},
     {"ima", "application/x-ima"},
     {"imap", "application/x-httpd-imap"},
     {"inf", "application/inf"},
     {"ins", "application/x-internet-signup"},
     {"ip", "application/x-ip2"},
     {"isu", "video/x-isvideo"},
     {"it", "audio/it"},
     {"iv", "application/x-inventor"},
     {"ivr", "i-world/i-vrml"},
     {"ivy", "application/x-livescreen"},
     {"jam", "audio/x-jam"},
     {"jav", "text/x-java-source"},
     {"java", "text/x-java-source"},
     {"jcm", "application/x-java-commerce"},
     {"jfif", "image/jpeg"},
     {"jp2", "image/jp2"},
     {"jfif-tbnl", "image/jpeg"},
     {"jpe", "image/jpeg"},
     {"jpeg", "image/jpeg"},
     {"jpg", "image/jpeg"},
     {"jps", "image/x-jps"},
     {"js", "application/javascript"},
     {"json", "application/json"},
     {"jut", "image/jutvision"},
     {"kar", "music/x-karaoke"},
     {"kdc", "image/kdc"},
     {"ksh", "text/x-script.ksh"},
     {"la", "audio/nspaudio"},
     {"lam", "audio/x-liveaudio"},
     {"latex", "application/x-latex"},
     {"lha", "application/lha"},
     {"lhx", "application/octet-stream"},
     {"list", "text/plain"},
     {"lma", "audio/nspaudio"},
     {"log", "text/plain"},
     {"lsp", "application/x-lisp"},
     {"lst", "text/plain"},
     {"lsx", "text/x-la-asf"},
     {"ltx", "application/x-latex"},
     {"lzh", "application/x-lzh"},
     {"lzx", "application/lzx"},
     {"m", "text/x-m"},
     {"m1v", "video/mpeg"},
     {"m2a", "audio/mpeg"},
     {"m2v", "video/mpeg"},
     {"m3u", "audio/x-mpegurl"},
     {"man", "application/x-troff-man"},
     {"map", "application/x-navimap"},
     {"mar", "text/plain"},
     {"mbd", "application/mbedlet"},
     {"mc$", "application/x-magic-cap-package-1.0"},
     {"mcd", "application/x-mathcad"},
     {"mcf", "text/mcf"},
     {"mcp", "application/netmc"},
     {"mdc", "image/mdc"},
     {"me", "application/x-troff-me"},
     {"mef", "image/mef"},
     {"mht", "message/rfc822"},
     {"mhtml", "message/rfc822"},
     {"mid", "audio/midi"},
     {"midi", "audio/midi"},
     {"mif", "application/x-mif"},
     {"mime", "message/rfc822"},
     {"mjf", "audio/x-vnd.audioexplosion.mjuicemediafile"},
     {"mjpg", "video/x-motion-jpeg"},
     {"mka", "audio/x-matroska"},
     {"mkv", "video/x-matroska"},
     {"mk3d", "video/x-matroska-3d"},
     {"mm", "application/x-meme"},
     {"mme", "application/base64"},
     {"mod", "audio/mod"},
     {"moov", "video/quicktime"},
     {"mov", "video/quicktime"},
     {"movie", "video/x-sgi-movie"},
     {"mos", "image/mos"},
     {"mp2", "audio/mpeg"},
     {"mp3", "audio/mpeg3"},
     {"mp4", "video/mp4"},
     {"mpa", "audio/mpeg"},
     {"mpc", "application/x-project"},
     {"mpe", "video/mpeg"},
     {"mpeg", "video/mpeg"},
     {"mpg", "video/mpeg"},
     {"mpga", "audio/mpeg"},
     {"mpp", "application/vnd.ms-project"},
     {"mpt", "application/x-project"},
     {"mpv", "application/x-project"},
     {"mpx", "application/x-project"},
     {"mrc", "application/marc"},
     {"mrw", "image/mrw"},
     {"ms", "application/x-troff-ms"},
     {"mv", "video/x-sgi-movie"},
     {"my", "audio/make"},
     {"mzz", "application/x-vnd.audioexplosion.mzz"},
     {"nap", "image/naplps"},
     {"naplps", "image/naplps"},
     {"nc", "application/x-netcdf"},
     {"ncm", "application/vnd.nokia.configuration-message"},
     {"nef", "image/nef"},
     {"nfo", "text/xml"},
     {"nif", "image/x-niff"},
     {"niff", "image/x-niff"},
     {"nix", "application/x-mix-transfer"},
     {"nrw", "image/nrw"},
     {"nsc", "application/x-conference"},
     {"nvd", "application/x-navidoc"},
     {"o", "application/octet-stream"},
     {"oda", "application/oda"},
     {"ogg", "audio/ogg"},
     {"omc", "application/x-omc"},
     {"omcd", "application/x-omcdatamaker"},
     {"omcr", "application/x-omcregerator"},
     {"orf", "image/orf"},
     {"p", "text/x-pascal"},
     {"p10", "application/pkcs10"},
     {"p12", "application/pkcs-12"},
     {"p7a", "application/x-pkcs7-signature"},
     {"p7c", "application/pkcs7-mime"},
     {"p7m", "application/pkcs7-mime"},
     {"p7r", "application/x-pkcs7-certreqresp"},
     {"p7s", "application/pkcs7-signature"},
     {"part", "application/pro_eng"},
     {"pas", "text/pascal"},
     {"pbm", "image/x-portable-bitmap"},
     {"pcl", "application/vnd.hp-pcl"},
     {"pct", "image/x-pict"},
     {"pcx", "image/x-pcx"},
     {"pdb", "chemical/x-pdb"},
     {"pdf", "application/pdf"},
     {"pef", "image/pef"},
     {"pfunk", "audio/make.my.funk"},
     {"pgm", "image/x-portable-greymap"},
     {"pic", "image/pict"},
     {"pict", "image/pict"},
     {"pkg", "application/x-newton-compatible-pkg"},
     {"pko", "application/vnd.ms-pki.pko"},
     {"pl", "text/x-script.perl"},
     {"plx", "application/x-pixclscript"},
     {"pm", "text/x-script.perl-module"},
     {"pm4", "application/x-pagemaker"},
     {"pm5", "application/x-pagemaker"},
     {"png", "image/png"},
     {"pnm", "application/x-portable-anymap"},
     {"pot", "application/vnd.ms-powerpoint"},
     {"pov", "model/x-pov"},
     {"ppa", "application/vnd.ms-powerpoint"},
     {"ppm", "image/x-portable-pixmap"},
     {"pps", "application/mspowerpoint"},
     {"ppt", "application/mspowerpoint"},
     {"ppz", "application/mspowerpoint"},
     {"pre", "application/x-freelance"},
     {"prt", "application/pro_eng"},
     {"ps", "application/postscript"},
     {"psd", "application/octet-stream"},
     {"pvu", "paleovu/x-pv"},
     {"pwz", "application/vnd.ms-powerpoint"},
     {"py", "text/x-script.python"},
     {"pyc", "application/x-bytecode.python"},
     {"qcp", "audio/vnd.qcelp"},
     {"qd3", "x-world/x-3dmf"},
     {"qd3d", "x-world/x-3dmf"},
     {"qif", "image/x-quicktime"},
     {"qt", "video/quicktime"},
     {"qtc", "video/x-qtc"},
     {"qti", "image/x-quicktime"},
     {"qtif", "image/x-quicktime"},
     {"ra", "audio/x-realaudio"},
     {"raf", "image/raf"},
     {"ram", "audio/x-pn-realaudio"},
     {"ras", "image/cmu-raster"},
     {"rast", "image/cmu-raster"},
     {"raw", "image/raw"},
     {"rexx", "text/x-script.rexx"},
     {"rf", "image/vnd.rn-realflash"},
     {"rgb", "image/x-rgb"},
     {"rm", "application/vnd.rn-realmedia"},
     {"rmi", "audio/mid"},
     {"rmm", "audio/x-pn-realaudio"},
     {"rmp", "audio/x-pn-realaudio"},
     {"rng", "application/ringing-tones"},
     {"rnx", "application/vnd.rn-realplayer"},
     {"roff", "application/x-troff"},
     {"rp", "image/vnd.rn-realpix"},
     {"rpm", "audio/x-pn-realaudio-plugin"},
     {"rt", "text/richtext"},
     {"rtf", "text/richtext"},
     {"rtx", "text/richtext"},
     {"rv", "video/vnd.rn-realvideo"},
     {"rw2", "image/rw2"},
     {"s", "text/x-asm"},
     {"s3m", "audio/s3m"},
     {"saveme", "application/octet-stream"},
     {"sbk", "application/x-tbook"},
     {"scm", "video/x-scm"},
     {"sdml", "text/plain"},
     {"sdp", "application/sdp"},
     {"sdr", "application/sounder"},
     {"sea", "application/sea"},
     {"set", "application/set"},
     {"sgm", "text/sgml"},
     {"sgml", "text/sgml"},
     {"sh", "text/x-script.sh"},
     {"shar", "application/x-bsh"},
     {"shtml", "text/x-server-parsed-html"},
     {"sid", "audio/x-psid"},
     {"sit", "application/x-stuffit"},
     {"skd", "application/x-koan"},
     {"skm", "application/x-koan"},
     {"skp", "application/x-koan"},
     {"skt", "application/x-koan"},
     {"sl", "application/x-seelogo"},
     {"smi", "application/smil"},
     {"smil", "application/smil"},
     {"snd", "audio/basic"},
     {"sol", "application/solids"},
     {"spc", "text/x-speech"},
     {"spl", "application/futuresplash"},
     {"spr", "application/x-sprite"},
     {"sprite", "application/x-sprite"},
     {"src", "application/x-wais-source"},
     {"srw", "image/srw"},
     {"ssi", "text/x-server-parsed-html"},
     {"ssm", "application/streamingmedia"},
     {"sst", "application/vnd.ms-pki.certstore"},
     {"step", "application/step"},
     {"stl", "application/sla"},
     {"stp", "application/step"},
     {"sup", "application/x-pgs"},
     {"sv4cpio", "application/x-sv4cpio"},
     {"sv4crc", "application/x-sv4crc"},
     {"svf", "image/vnd.dwg"},
     {"svg", "image/svg+xml"},
     {"svr", "application/x-world"},
     {"swf", "application/x-shockwave-flash"},
     {"t", "application/x-troff"},
     {"talk", "text/x-speech"},
     {"tar", "application/x-tar"},
     {"tbk", "application/toolbook"},
     {"tcl", "text/x-script.tcl"},
     {"tcsh", "text/x-script.tcsh"},
     {"tex", "application/x-tex"},
     {"texi", "application/x-texinfo"},
     {"texinfo", "application/x-texinfo"},
     {"text", "text/plain"},
     {"tgz", "application/x-compressed"},
     {"tif", "image/tiff"},
     {"tiff", "image/tiff"},
     {"tr", "application/x-troff"},
     {"ts", "video/mp2t"},
     {"tsi", "audio/tsp-audio"},
     {"tsp", "audio/tsplayer"},
     {"tsv", "text/tab-separated-values"},
     {"turbot", "image/florian"},
     {"txt", "text/plain"},
     {"uil", "text/x-uil"},
     {"uni", "text/uri-list"},
     {"unis", "text/uri-list"},
     {"unv", "application/i-deas"},
     {"uri", "text/uri-list"},
     {"uris", "text/uri-list"},
     {"ustar", "application/x-ustar"},
     {"uu", "text/x-uuencode"},
     {"uue", "text/x-uuencode"},
     {"vcd", "application/x-cdlink"},
     {"vcs", "text/x-vcalendar"},
     {"vda", "application/vda"},
     {"vdo", "video/vdo"},
     {"vew", "application/groupwise"},
     {"viv", "video/vivo"},
     {"vivo", "video/vivo"},
     {"vmd", "application/vocaltec-media-desc"},
     {"vmf", "application/vocaltec-media-file"},
     {"voc", "audio/voc"},
     {"vos", "video/vosaic"},
     {"vox", "audio/voxware"},
     {"vqe", "audio/x-twinvq-plugin"},
     {"vqf", "audio/x-twinvq"},
     {"vql", "audio/x-twinvq-plugin"},
     {"vrml", "application/x-vrml"},
     {"vrt", "x-world/x-vrt"},
     {"vsd", "application/x-visio"},
     {"vst", "application/x-visio"},
     {"vsw", "application/x-visio"},
     {"vtt", "text/vtt"},
     {"w60", "application/wordperfect6.0"},
     {"w61", "application/wordperfect6.1"},
     {"w6w", "application/msword"},
     {"wav", "audio/wav"},
     {"wb1", "application/x-qpro"},
     {"wbmp", "image/vnd.wap.wbmp"},
     {"web", "application/vnd.xara"},
     {"webp", "image/webp"},
     {"wiz", "application/msword"},
     {"wk1", "application/x-123"},
     {"wma", "audio/x-ms-wma"},
     {"wmf", "windows/metafile"},
     {"wml", "text/vnd.wap.wml"},
     {"wmlc", "application/vnd.wap.wmlc"},
     {"wmls", "text/vnd.wap.wmlscript"},
     {"wmlsc", "application/vnd.wap.wmlscriptc"},
     {"wmv", "video/x-ms-wmv"},
     {"word", "application/msword"},
     {"wp", "application/wordperfect"},
     {"wp5", "application/wordperfect"},
     {"wp6", "application/wordperfect"},
     {"wpd", "application/wordperfect"},
     {"wq1", "application/x-lotus"},
     {"wri", "application/mswrite"},
     {"wrl", "model/vrml"},
     {"wrz", "model/vrml"},
     {"wsc", "text/scriplet"},
     {"wsrc", "application/x-wais-source"},
     {"wtk", "application/x-wintalk"},
     {"x3f", "image/x3f"},
     {"xbm", "image/xbm"},
     {"xdr", "video/x-amt-demorun"},
     {"xgz", "xgl/drawing"},
     {"xif", "image/vnd.xiff"},
     {"xl", "application/excel"},
     {"xla", "application/excel"},
     {"xlb", "application/excel"},
     {"xlc", "application/excel"},
     {"xld", "application/excel"},
     {"xlk", "application/excel"},
     {"xll", "application/excel"},
     {"xlm", "application/excel"},
     {"xls", "application/excel"},
     {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
     {"xlt", "application/excel"},
     {"xlv", "application/excel"},
     {"xlw", "application/excel"},
     {"xm", "audio/xm"},
     {"xml", "text/xml"},
     {"xmz", "xgl/movie"},
     {"xpix", "application/x-vnd.ls-xpix"},
     {"xpm", "image/xpm"},
     {"x-png", "image/png"},
     {"xspf", "application/xspf+xml"},
     {"xsr", "video/x-amt-showrun"},
     {"xvid", "video/x-msvideo"},
     {"xwd", "image/x-xwd"},
     {"xyz", "chemical/x-pdb"},
     {"z", "application/x-compressed"},
     {"zip", "application/zip"},
     {"zoo", "application/octet-stream"},
     {"zsh", "text/x-script.zsh"}}};

std::string CMime::GetMimeType(const std::string &extension)
{
  if (extension.empty())
    return "";

  std::string ext = extension;
  size_t posNotPoint = ext.find_first_not_of('.');
  if (posNotPoint != std::string::npos && posNotPoint > 0)
    ext = extension.substr(posNotPoint);
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  std::map<std::string, std::string>::const_iterator it = m_mimetypes.find(ext);
  if (it != m_mimetypes.end())
    return it->second;

  return "";
}

std::string CMime::GetMimeType(const CFileItem &item)
{
  std::string path = item.GetDynPath();
  if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->GetPath().empty())
    path = item.GetVideoInfoTag()->GetPath();
  else if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->GetURL().empty())
    path = item.GetMusicInfoTag()->GetURL();

  return GetMimeType(URIUtils::GetExtension(path));
}

std::string CMime::GetMimeType(const CURL &url, bool lookup)
{

  std::string strMimeType;

  if( url.IsProtocol("shout") || url.IsProtocol("http") || url.IsProtocol("https"))
  {
    // If lookup is false, bail out early to leave mime type empty
    if (!lookup)
      return strMimeType;

    std::string strmime;
    XFILE::CCurlFile::GetMimeType(url, strmime);

    // try to get mime-type again but with an NSPlayer User-Agent
    // in order for server to provide correct mime-type.  Allows us
    // to properly detect an MMS stream
    if (StringUtils::StartsWithNoCase(strmime, "video/x-ms-"))
      XFILE::CCurlFile::GetMimeType(url, strmime, "NSPlayer/11.00.6001.7000");

    // make sure there are no options set in mime-type
    // mime-type can look like "video/x-ms-asf ; charset=utf8"
    size_t i = strmime.find(';');
    if(i != std::string::npos)
      strmime.erase(i, strmime.length() - i);
    StringUtils::Trim(strmime);
    strMimeType = strmime;
  }
  else
    strMimeType = GetMimeType(url.GetFileType());

  // if it's still empty set to an unknown type
  if (strMimeType.empty())
    strMimeType = "application/octet-stream";

  return strMimeType;
}

CMime::EFileType CMime::GetFileTypeFromMime(const std::string& mimeType)
{
  // based on http://mimesniff.spec.whatwg.org/

  std::string type, subtype;
  if (!parseMimeType(mimeType, type, subtype))
    return FileTypeUnknown;

  if (type == "application")
  {
    if (subtype == "zip")
      return FileTypeZip;
    if (subtype == "x-gzip")
      return FileTypeGZip;
    if (subtype == "x-rar-compressed")
      return FileTypeRar;

    if (subtype == "xml")
      return FileTypeXml;
  }
  else if (type == "text")
  {
    if (subtype == "xml")
      return FileTypeXml;
    if (subtype == "html")
      return FileTypeHtml;
    if (subtype == "plain")
      return FileTypePlainText;
  }
  else if (type == "image")
  {
    if (subtype == "bmp")
      return FileTypeBmp;
    if (subtype == "gif")
      return FileTypeGif;
    if (subtype == "png")
      return FileTypePng;
    if (subtype == "jpeg" || subtype == "pjpeg")
      return FileTypeJpeg;
  }

  if (StringUtils::EndsWith(subtype, "+zip"))
    return FileTypeZip;
  if (StringUtils::EndsWith(subtype, "+xml"))
    return FileTypeXml;

  return FileTypeUnknown;
}

CMime::EFileType CMime::GetFileTypeFromContent(const std::string& fileContent)
{
  // based on http://mimesniff.spec.whatwg.org/#matching-a-mime-type-pattern

  const size_t len = fileContent.length();
  if (len < 2)
    return FileTypeUnknown;

  const unsigned char* const b = (const unsigned char*)fileContent.c_str();

  //! @todo add detection for text types

  // check image types
  if (b[0] == 'B' && b[1] == 'M')
    return FileTypeBmp;
  if (len >= 6 && b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8' && (b[4] == '7' || b[4] == '9') && b[5] == 'a')
    return FileTypeGif;
  if (len >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' && b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A)
    return FileTypePng;
  if (len >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF)
    return FileTypeJpeg;

  // check archive types
  if (len >= 3 && b[0] == 0x1F && b[1] == 0x8B && b[2] == 0x08)
    return FileTypeGZip;
  if (len >= 4 && b[0] == 'P' && b[1] == 'K' && b[2] == 0x03 && b[3] == 0x04)
    return FileTypeZip;
  if (len >= 7 && b[0] == 'R' && b[1] == 'a' && b[2] == 'r' && b[3] == ' ' && b[4] == 0x1A && b[5] == 0x07 && b[6] == 0x00)
    return FileTypeRar;

  //! @todo add detection for other types if required

  return FileTypeUnknown;
}

bool CMime::parseMimeType(const std::string& mimeType, std::string& type, std::string& subtype)
{
  static const char* const whitespaceChars = "\x09\x0A\x0C\x0D\x20"; // tab, LF, FF, CR and space

  type.clear();
  subtype.clear();

  const size_t slashPos = mimeType.find('/');
  if (slashPos == std::string::npos)
    return false;

  type.assign(mimeType, 0, slashPos);
  subtype.assign(mimeType, slashPos + 1, std::string::npos);

  const size_t semicolonPos = subtype.find(';');
  if (semicolonPos != std::string::npos)
    subtype.erase(semicolonPos);

  StringUtils::Trim(type, whitespaceChars);
  StringUtils::Trim(subtype, whitespaceChars);

  if (type.empty() || subtype.empty())
  {
    type.clear();
    subtype.clear();
    return false;
  }

  StringUtils::ToLower(type);
  StringUtils::ToLower(subtype);

  return true;
}

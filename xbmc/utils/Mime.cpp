/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include <algorithm>

#include "Mime.h"
#include "FileItem.h"
#include "URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "video/VideoInfoTag.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "filesystem/CurlFile.h"

std::map<std::string, std::string> fillMimeTypes()
{
  std::map<std::string, std::string> mimeTypes;

  mimeTypes.insert(std::pair<std::string, std::string>("3dm",       "x-world/x-3dmf"));
  mimeTypes.insert(std::pair<std::string, std::string>("3dmf",      "x-world/x-3dmf"));
  mimeTypes.insert(std::pair<std::string, std::string>("a",         "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("aab",       "application/x-authorware-bin"));
  mimeTypes.insert(std::pair<std::string, std::string>("aam",       "application/x-authorware-map"));
  mimeTypes.insert(std::pair<std::string, std::string>("aas",       "application/x-authorware-seg"));
  mimeTypes.insert(std::pair<std::string, std::string>("abc",       "text/vnd.abc"));
  mimeTypes.insert(std::pair<std::string, std::string>("acgi",      "text/html"));
  mimeTypes.insert(std::pair<std::string, std::string>("afl",       "video/animaflex"));
  mimeTypes.insert(std::pair<std::string, std::string>("ai",        "application/postscript"));
  mimeTypes.insert(std::pair<std::string, std::string>("aif",       "audio/aiff"));
  mimeTypes.insert(std::pair<std::string, std::string>("aifc",      "audio/x-aiff"));
  mimeTypes.insert(std::pair<std::string, std::string>("aiff",      "audio/aiff"));
  mimeTypes.insert(std::pair<std::string, std::string>("aim",       "application/x-aim"));
  mimeTypes.insert(std::pair<std::string, std::string>("aip",       "text/x-audiosoft-intra"));
  mimeTypes.insert(std::pair<std::string, std::string>("ani",       "application/x-navi-animation"));
  mimeTypes.insert(std::pair<std::string, std::string>("aos",       "application/x-nokia-9000-communicator-add-on-software"));
  mimeTypes.insert(std::pair<std::string, std::string>("apng",      "image/apng"));
  mimeTypes.insert(std::pair<std::string, std::string>("aps",       "application/mime"));
  mimeTypes.insert(std::pair<std::string, std::string>("arc",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("arj",       "application/arj"));
  mimeTypes.insert(std::pair<std::string, std::string>("art",       "image/x-jg"));
  mimeTypes.insert(std::pair<std::string, std::string>("asf",       "video/x-ms-asf"));
  mimeTypes.insert(std::pair<std::string, std::string>("asm",       "text/x-asm"));
  mimeTypes.insert(std::pair<std::string, std::string>("asp",       "text/asp"));
  mimeTypes.insert(std::pair<std::string, std::string>("asx",       "video/x-ms-asf"));
  mimeTypes.insert(std::pair<std::string, std::string>("au",        "audio/basic"));
  mimeTypes.insert(std::pair<std::string, std::string>("avi",       "video/avi"));
  mimeTypes.insert(std::pair<std::string, std::string>("avs",       "video/avs-video"));
  mimeTypes.insert(std::pair<std::string, std::string>("bcpio",     "application/x-bcpio"));
  mimeTypes.insert(std::pair<std::string, std::string>("bin",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("bm",        "image/bmp"));
  mimeTypes.insert(std::pair<std::string, std::string>("bmp",       "image/bmp"));
  mimeTypes.insert(std::pair<std::string, std::string>("boo",       "application/book"));
  mimeTypes.insert(std::pair<std::string, std::string>("book",      "application/book"));
  mimeTypes.insert(std::pair<std::string, std::string>("boz",       "application/x-bzip2"));
  mimeTypes.insert(std::pair<std::string, std::string>("bsh",       "application/x-bsh"));
  mimeTypes.insert(std::pair<std::string, std::string>("bz",        "application/x-bzip"));
  mimeTypes.insert(std::pair<std::string, std::string>("bz2",       "application/x-bzip2"));
  mimeTypes.insert(std::pair<std::string, std::string>("c",         "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("c++",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("cat",       "application/vnd.ms-pki.seccat"));
  mimeTypes.insert(std::pair<std::string, std::string>("cc",        "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("ccad",      "application/clariscad"));
  mimeTypes.insert(std::pair<std::string, std::string>("cco",       "application/x-cocoa"));
  mimeTypes.insert(std::pair<std::string, std::string>("cdf",       "application/cdf"));
  mimeTypes.insert(std::pair<std::string, std::string>("cer",       "application/pkix-cert"));
  mimeTypes.insert(std::pair<std::string, std::string>("cer",       "application/x-x509-ca-cert"));
  mimeTypes.insert(std::pair<std::string, std::string>("cha",       "application/x-chat"));
  mimeTypes.insert(std::pair<std::string, std::string>("chat",      "application/x-chat"));
  mimeTypes.insert(std::pair<std::string, std::string>("class",     "application/java"));
  mimeTypes.insert(std::pair<std::string, std::string>("com",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("conf",      "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("cpio",      "application/x-cpio"));
  mimeTypes.insert(std::pair<std::string, std::string>("cpp",       "text/x-c"));
  mimeTypes.insert(std::pair<std::string, std::string>("cpt",       "application/x-cpt"));
  mimeTypes.insert(std::pair<std::string, std::string>("crl",       "application/pkcs-crl"));
  mimeTypes.insert(std::pair<std::string, std::string>("crt",       "application/pkix-cert"));
  mimeTypes.insert(std::pair<std::string, std::string>("csh",       "application/x-csh"));
  mimeTypes.insert(std::pair<std::string, std::string>("css",       "text/css"));
  mimeTypes.insert(std::pair<std::string, std::string>("cxx",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("dcr",       "application/x-director"));
  mimeTypes.insert(std::pair<std::string, std::string>("deepv",     "application/x-deepv"));
  mimeTypes.insert(std::pair<std::string, std::string>("def",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("der",       "application/x-x509-ca-cert"));
  mimeTypes.insert(std::pair<std::string, std::string>("dif",       "video/x-dv"));
  mimeTypes.insert(std::pair<std::string, std::string>("dir",       "application/x-director"));
  mimeTypes.insert(std::pair<std::string, std::string>("dl",        "video/dl"));
  mimeTypes.insert(std::pair<std::string, std::string>("divx",      "video/x-msvideo"));
  mimeTypes.insert(std::pair<std::string, std::string>("doc",       "application/msword"));
  mimeTypes.insert(std::pair<std::string, std::string>("docx",      "application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
  mimeTypes.insert(std::pair<std::string, std::string>("dot",       "application/msword"));
  mimeTypes.insert(std::pair<std::string, std::string>("dp",        "application/commonground"));
  mimeTypes.insert(std::pair<std::string, std::string>("drw",       "application/drafting"));
  mimeTypes.insert(std::pair<std::string, std::string>("dump",      "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("dv",        "video/x-dv"));
  mimeTypes.insert(std::pair<std::string, std::string>("dvi",       "application/x-dvi"));
  mimeTypes.insert(std::pair<std::string, std::string>("dwf",       "model/vnd.dwf"));
  mimeTypes.insert(std::pair<std::string, std::string>("dwg",       "image/vnd.dwg"));
  mimeTypes.insert(std::pair<std::string, std::string>("dxf",       "image/vnd.dwg"));
  mimeTypes.insert(std::pair<std::string, std::string>("dxr",       "application/x-director"));
  mimeTypes.insert(std::pair<std::string, std::string>("el",        "text/x-script.elisp"));
  mimeTypes.insert(std::pair<std::string, std::string>("elc",       "application/x-elc"));
  mimeTypes.insert(std::pair<std::string, std::string>("env",       "application/x-envoy"));
  mimeTypes.insert(std::pair<std::string, std::string>("eps",       "application/postscript"));
  mimeTypes.insert(std::pair<std::string, std::string>("es",        "application/x-esrehber"));
  mimeTypes.insert(std::pair<std::string, std::string>("etx",       "text/x-setext"));
  mimeTypes.insert(std::pair<std::string, std::string>("evy",       "application/envoy"));
  mimeTypes.insert(std::pair<std::string, std::string>("exe",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("f",         "text/x-fortran"));
  mimeTypes.insert(std::pair<std::string, std::string>("f77",       "text/x-fortran"));
  mimeTypes.insert(std::pair<std::string, std::string>("f90",       "text/x-fortran"));
  mimeTypes.insert(std::pair<std::string, std::string>("fdf",       "application/vnd.fdf"));
  mimeTypes.insert(std::pair<std::string, std::string>("fif",       "image/fif"));
  mimeTypes.insert(std::pair<std::string, std::string>("flac",      "audio/flac"));
  mimeTypes.insert(std::pair<std::string, std::string>("fli",       "video/fli"));
  mimeTypes.insert(std::pair<std::string, std::string>("flo",       "image/florian"));
  mimeTypes.insert(std::pair<std::string, std::string>("flv",       "video/x-flv"));
  mimeTypes.insert(std::pair<std::string, std::string>("flx",       "text/vnd.fmi.flexstor"));
  mimeTypes.insert(std::pair<std::string, std::string>("fmf",       "video/x-atomic3d-feature"));
  mimeTypes.insert(std::pair<std::string, std::string>("for",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("for",       "text/x-fortran"));
  mimeTypes.insert(std::pair<std::string, std::string>("fpx",       "image/vnd.fpx"));
  mimeTypes.insert(std::pair<std::string, std::string>("frl",       "application/freeloader"));
  mimeTypes.insert(std::pair<std::string, std::string>("funk",      "audio/make"));
  mimeTypes.insert(std::pair<std::string, std::string>("g",         "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("g3",        "image/g3fax"));
  mimeTypes.insert(std::pair<std::string, std::string>("gif",       "image/gif"));
  mimeTypes.insert(std::pair<std::string, std::string>("gl",        "video/x-gl"));
  mimeTypes.insert(std::pair<std::string, std::string>("gsd",       "audio/x-gsm"));
  mimeTypes.insert(std::pair<std::string, std::string>("gsm",       "audio/x-gsm"));
  mimeTypes.insert(std::pair<std::string, std::string>("gsp",       "application/x-gsp"));
  mimeTypes.insert(std::pair<std::string, std::string>("gss",       "application/x-gss"));
  mimeTypes.insert(std::pair<std::string, std::string>("gtar",      "application/x-gtar"));
  mimeTypes.insert(std::pair<std::string, std::string>("gz",        "application/x-compressed"));
  mimeTypes.insert(std::pair<std::string, std::string>("gzip",      "application/x-gzip"));
  mimeTypes.insert(std::pair<std::string, std::string>("h",         "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("hdf",       "application/x-hdf"));
  mimeTypes.insert(std::pair<std::string, std::string>("help",      "application/x-helpfile"));
  mimeTypes.insert(std::pair<std::string, std::string>("hgl",       "application/vnd.hp-hpgl"));
  mimeTypes.insert(std::pair<std::string, std::string>("hh",        "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("hlb",       "text/x-script"));
  mimeTypes.insert(std::pair<std::string, std::string>("hlp",       "application/hlp"));
  mimeTypes.insert(std::pair<std::string, std::string>("hpg",       "application/vnd.hp-hpgl"));
  mimeTypes.insert(std::pair<std::string, std::string>("hpgl",      "application/vnd.hp-hpgl"));
  mimeTypes.insert(std::pair<std::string, std::string>("hqx",       "application/binhex"));
  mimeTypes.insert(std::pair<std::string, std::string>("hta",       "application/hta"));
  mimeTypes.insert(std::pair<std::string, std::string>("htc",       "text/x-component"));
  mimeTypes.insert(std::pair<std::string, std::string>("htm",       "text/html"));
  mimeTypes.insert(std::pair<std::string, std::string>("html",      "text/html"));
  mimeTypes.insert(std::pair<std::string, std::string>("htmls",     "text/html"));
  mimeTypes.insert(std::pair<std::string, std::string>("htt",       "text/webviewhtml"));
  mimeTypes.insert(std::pair<std::string, std::string>("htx",       "text/html"));
  mimeTypes.insert(std::pair<std::string, std::string>("ice",       "x-conference/x-cooltalk"));
  mimeTypes.insert(std::pair<std::string, std::string>("ico",       "image/x-icon"));
  mimeTypes.insert(std::pair<std::string, std::string>("idc",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("ief",       "image/ief"));
  mimeTypes.insert(std::pair<std::string, std::string>("iefs",      "image/ief"));
  mimeTypes.insert(std::pair<std::string, std::string>("iges",      "application/iges"));
  mimeTypes.insert(std::pair<std::string, std::string>("igs",       "application/iges"));
  mimeTypes.insert(std::pair<std::string, std::string>("igs",       "model/iges"));
  mimeTypes.insert(std::pair<std::string, std::string>("ima",       "application/x-ima"));
  mimeTypes.insert(std::pair<std::string, std::string>("imap",      "application/x-httpd-imap"));
  mimeTypes.insert(std::pair<std::string, std::string>("inf",       "application/inf"));
  mimeTypes.insert(std::pair<std::string, std::string>("ins",       "application/x-internett-signup"));
  mimeTypes.insert(std::pair<std::string, std::string>("ip",        "application/x-ip2"));
  mimeTypes.insert(std::pair<std::string, std::string>("isu",       "video/x-isvideo"));
  mimeTypes.insert(std::pair<std::string, std::string>("it",        "audio/it"));
  mimeTypes.insert(std::pair<std::string, std::string>("iv",        "application/x-inventor"));
  mimeTypes.insert(std::pair<std::string, std::string>("ivr",       "i-world/i-vrml"));
  mimeTypes.insert(std::pair<std::string, std::string>("ivy",       "application/x-livescreen"));
  mimeTypes.insert(std::pair<std::string, std::string>("jam",       "audio/x-jam"));
  mimeTypes.insert(std::pair<std::string, std::string>("jav",       "text/x-java-source"));
  mimeTypes.insert(std::pair<std::string, std::string>("java",      "text/x-java-source"));
  mimeTypes.insert(std::pair<std::string, std::string>("jcm",       "application/x-java-commerce"));
  mimeTypes.insert(std::pair<std::string, std::string>("jfif",      "image/jpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("jp2",       "image/jp2"));
  mimeTypes.insert(std::pair<std::string, std::string>("jfif-tbnl", "image/jpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("jpe",       "image/jpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("jpeg",      "image/jpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("jpg",       "image/jpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("jps",       "image/x-jps"));
  mimeTypes.insert(std::pair<std::string, std::string>("js",        "application/javascript"));
  mimeTypes.insert(std::pair<std::string, std::string>("json",      "application/json"));
  mimeTypes.insert(std::pair<std::string, std::string>("jut",       "image/jutvision"));
  mimeTypes.insert(std::pair<std::string, std::string>("kar",       "music/x-karaoke"));
  mimeTypes.insert(std::pair<std::string, std::string>("ksh",       "application/x-ksh"));
  mimeTypes.insert(std::pair<std::string, std::string>("ksh",       "text/x-script.ksh"));
  mimeTypes.insert(std::pair<std::string, std::string>("la",        "audio/nspaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("lam",       "audio/x-liveaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("latex",     "application/x-latex"));
  mimeTypes.insert(std::pair<std::string, std::string>("lha",       "application/lha"));
  mimeTypes.insert(std::pair<std::string, std::string>("lhx",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("list",      "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("lma",       "audio/nspaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("log",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("lsp",       "application/x-lisp"));
  mimeTypes.insert(std::pair<std::string, std::string>("lst",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("lsx",       "text/x-la-asf"));
  mimeTypes.insert(std::pair<std::string, std::string>("ltx",       "application/x-latex"));
  mimeTypes.insert(std::pair<std::string, std::string>("lzh",       "application/x-lzh"));
  mimeTypes.insert(std::pair<std::string, std::string>("lzx",       "application/lzx"));
  mimeTypes.insert(std::pair<std::string, std::string>("m",         "text/x-m"));
  mimeTypes.insert(std::pair<std::string, std::string>("m1v",       "video/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("m2a",       "audio/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("m2v",       "video/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("m3u",       "audio/x-mpequrl"));
  mimeTypes.insert(std::pair<std::string, std::string>("man",       "application/x-troff-man"));
  mimeTypes.insert(std::pair<std::string, std::string>("map",       "application/x-navimap"));
  mimeTypes.insert(std::pair<std::string, std::string>("mar",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("mbd",       "application/mbedlet"));
  mimeTypes.insert(std::pair<std::string, std::string>("mc$",       "application/x-magic-cap-package-1.0"));
  mimeTypes.insert(std::pair<std::string, std::string>("mcd",       "application/x-mathcad"));
  mimeTypes.insert(std::pair<std::string, std::string>("mcf",       "text/mcf"));
  mimeTypes.insert(std::pair<std::string, std::string>("mcp",       "application/netmc"));
  mimeTypes.insert(std::pair<std::string, std::string>("me",        "application/x-troff-me"));
  mimeTypes.insert(std::pair<std::string, std::string>("mht",       "message/rfc822"));
  mimeTypes.insert(std::pair<std::string, std::string>("mhtml",     "message/rfc822"));
  mimeTypes.insert(std::pair<std::string, std::string>("mid",       "audio/midi"));
  mimeTypes.insert(std::pair<std::string, std::string>("midi",      "audio/midi"));
  mimeTypes.insert(std::pair<std::string, std::string>("mif",       "application/x-mif"));
  mimeTypes.insert(std::pair<std::string, std::string>("mime",      "message/rfc822"));
  mimeTypes.insert(std::pair<std::string, std::string>("mjf",       "audio/x-vnd.audioexplosion.mjuicemediafile"));
  mimeTypes.insert(std::pair<std::string, std::string>("mjpg",      "video/x-motion-jpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mka",       "audio/x-matroska"));
  mimeTypes.insert(std::pair<std::string, std::string>("mkv",       "video/x-matroska"));
  mimeTypes.insert(std::pair<std::string, std::string>("mk3d",      "video/x-matroska-3d"));
  mimeTypes.insert(std::pair<std::string, std::string>("mm",        "application/x-meme"));
  mimeTypes.insert(std::pair<std::string, std::string>("mme",       "application/base64"));
  mimeTypes.insert(std::pair<std::string, std::string>("mod",       "audio/mod"));
  mimeTypes.insert(std::pair<std::string, std::string>("moov",      "video/quicktime"));
  mimeTypes.insert(std::pair<std::string, std::string>("mov",       "video/quicktime"));
  mimeTypes.insert(std::pair<std::string, std::string>("movie",     "video/x-sgi-movie"));
  mimeTypes.insert(std::pair<std::string, std::string>("mp2",       "audio/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mp3",       "audio/mpeg3"));
  mimeTypes.insert(std::pair<std::string, std::string>("mp4",       "video/mp4"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpa",       "audio/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpc",       "application/x-project"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpe",       "video/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpeg",      "video/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpg",       "video/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpga",      "audio/mpeg"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpp",       "application/vnd.ms-project"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpt",       "application/x-project"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpv",       "application/x-project"));
  mimeTypes.insert(std::pair<std::string, std::string>("mpx",       "application/x-project"));
  mimeTypes.insert(std::pair<std::string, std::string>("mrc",       "application/marc"));
  mimeTypes.insert(std::pair<std::string, std::string>("ms",        "application/x-troff-ms"));
  mimeTypes.insert(std::pair<std::string, std::string>("mv",        "video/x-sgi-movie"));
  mimeTypes.insert(std::pair<std::string, std::string>("my",        "audio/make"));
  mimeTypes.insert(std::pair<std::string, std::string>("mzz",       "application/x-vnd.audioexplosion.mzz"));
  mimeTypes.insert(std::pair<std::string, std::string>("nap",       "image/naplps"));
  mimeTypes.insert(std::pair<std::string, std::string>("naplps",    "image/naplps"));
  mimeTypes.insert(std::pair<std::string, std::string>("nc",        "application/x-netcdf"));
  mimeTypes.insert(std::pair<std::string, std::string>("ncm",       "application/vnd.nokia.configuration-message"));
  mimeTypes.insert(std::pair<std::string, std::string>("nfo",       "text/xml"));
  mimeTypes.insert(std::pair<std::string, std::string>("nif",       "image/x-niff"));
  mimeTypes.insert(std::pair<std::string, std::string>("niff",      "image/x-niff"));
  mimeTypes.insert(std::pair<std::string, std::string>("nix",       "application/x-mix-transfer"));
  mimeTypes.insert(std::pair<std::string, std::string>("nsc",       "application/x-conference"));
  mimeTypes.insert(std::pair<std::string, std::string>("nvd",       "application/x-navidoc"));
  mimeTypes.insert(std::pair<std::string, std::string>("o",         "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("oda",       "application/oda"));
  mimeTypes.insert(std::pair<std::string, std::string>("ogg",       "audio/ogg"));
  mimeTypes.insert(std::pair<std::string, std::string>("omc",       "application/x-omc"));
  mimeTypes.insert(std::pair<std::string, std::string>("omcd",      "application/x-omcdatamaker"));
  mimeTypes.insert(std::pair<std::string, std::string>("omcr",      "application/x-omcregerator"));
  mimeTypes.insert(std::pair<std::string, std::string>("p",         "text/x-pascal"));
  mimeTypes.insert(std::pair<std::string, std::string>("p10",       "application/pkcs10"));
  mimeTypes.insert(std::pair<std::string, std::string>("p12",       "application/pkcs-12"));
  mimeTypes.insert(std::pair<std::string, std::string>("p7a",       "application/x-pkcs7-signature"));
  mimeTypes.insert(std::pair<std::string, std::string>("p7c",       "application/pkcs7-mime"));
  mimeTypes.insert(std::pair<std::string, std::string>("p7m",       "application/pkcs7-mime"));
  mimeTypes.insert(std::pair<std::string, std::string>("p7r",       "application/x-pkcs7-certreqresp"));
  mimeTypes.insert(std::pair<std::string, std::string>("p7s",       "application/pkcs7-signature"));
  mimeTypes.insert(std::pair<std::string, std::string>("part",      "application/pro_eng"));
  mimeTypes.insert(std::pair<std::string, std::string>("pas",       "text/pascal"));
  mimeTypes.insert(std::pair<std::string, std::string>("pbm",       "image/x-portable-bitmap"));
  mimeTypes.insert(std::pair<std::string, std::string>("pcl",       "application/vnd.hp-pcl"));
  mimeTypes.insert(std::pair<std::string, std::string>("pct",       "image/x-pict"));
  mimeTypes.insert(std::pair<std::string, std::string>("pcx",       "image/x-pcx"));
  mimeTypes.insert(std::pair<std::string, std::string>("pdb",       "chemical/x-pdb"));
  mimeTypes.insert(std::pair<std::string, std::string>("pdf",       "application/pdf"));
  mimeTypes.insert(std::pair<std::string, std::string>("pfunk",     "audio/make.my.funk"));
  mimeTypes.insert(std::pair<std::string, std::string>("pgm",       "image/x-portable-greymap"));
  mimeTypes.insert(std::pair<std::string, std::string>("pic",       "image/pict"));
  mimeTypes.insert(std::pair<std::string, std::string>("pict",      "image/pict"));
  mimeTypes.insert(std::pair<std::string, std::string>("pkg",       "application/x-newton-compatible-pkg"));
  mimeTypes.insert(std::pair<std::string, std::string>("pko",       "application/vnd.ms-pki.pko"));
  mimeTypes.insert(std::pair<std::string, std::string>("pl",        "text/x-script.perl"));
  mimeTypes.insert(std::pair<std::string, std::string>("plx",       "application/x-pixclscript"));
  mimeTypes.insert(std::pair<std::string, std::string>("pm",        "text/x-script.perl-module"));
  mimeTypes.insert(std::pair<std::string, std::string>("pm4",       "application/x-pagemaker"));
  mimeTypes.insert(std::pair<std::string, std::string>("pm5",       "application/x-pagemaker"));
  mimeTypes.insert(std::pair<std::string, std::string>("png",       "image/png"));
  mimeTypes.insert(std::pair<std::string, std::string>("pnm",       "application/x-portable-anymap"));
  mimeTypes.insert(std::pair<std::string, std::string>("pot",       "application/vnd.ms-powerpoint"));
  mimeTypes.insert(std::pair<std::string, std::string>("pov",       "model/x-pov"));
  mimeTypes.insert(std::pair<std::string, std::string>("ppa",       "application/vnd.ms-powerpoint"));
  mimeTypes.insert(std::pair<std::string, std::string>("ppm",       "image/x-portable-pixmap"));
  mimeTypes.insert(std::pair<std::string, std::string>("pps",       "application/mspowerpoint"));
  mimeTypes.insert(std::pair<std::string, std::string>("ppt",       "application/mspowerpoint"));
  mimeTypes.insert(std::pair<std::string, std::string>("ppz",       "application/mspowerpoint"));
  mimeTypes.insert(std::pair<std::string, std::string>("pre",       "application/x-freelance"));
  mimeTypes.insert(std::pair<std::string, std::string>("prt",       "application/pro_eng"));
  mimeTypes.insert(std::pair<std::string, std::string>("ps",        "application/postscript"));
  mimeTypes.insert(std::pair<std::string, std::string>("psd",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("pvu",       "paleovu/x-pv"));
  mimeTypes.insert(std::pair<std::string, std::string>("pwz",       "application/vnd.ms-powerpoint"));
  mimeTypes.insert(std::pair<std::string, std::string>("py",        "text/x-script.phyton"));
  mimeTypes.insert(std::pair<std::string, std::string>("pyc",       "applicaiton/x-bytecode.python"));
  mimeTypes.insert(std::pair<std::string, std::string>("qcp",       "audio/vnd.qcelp"));
  mimeTypes.insert(std::pair<std::string, std::string>("qd3",       "x-world/x-3dmf"));
  mimeTypes.insert(std::pair<std::string, std::string>("qd3d",      "x-world/x-3dmf"));
  mimeTypes.insert(std::pair<std::string, std::string>("qif",       "image/x-quicktime"));
  mimeTypes.insert(std::pair<std::string, std::string>("qt",        "video/quicktime"));
  mimeTypes.insert(std::pair<std::string, std::string>("qtc",       "video/x-qtc"));
  mimeTypes.insert(std::pair<std::string, std::string>("qti",       "image/x-quicktime"));
  mimeTypes.insert(std::pair<std::string, std::string>("qtif",      "image/x-quicktime"));
  mimeTypes.insert(std::pair<std::string, std::string>("ra",        "audio/x-realaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("ram",       "audio/x-pn-realaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("ras",       "image/cmu-raster"));
  mimeTypes.insert(std::pair<std::string, std::string>("rast",      "image/cmu-raster"));
  mimeTypes.insert(std::pair<std::string, std::string>("rexx",      "text/x-script.rexx"));
  mimeTypes.insert(std::pair<std::string, std::string>("rf",        "image/vnd.rn-realflash"));
  mimeTypes.insert(std::pair<std::string, std::string>("rgb",       "image/x-rgb"));
  mimeTypes.insert(std::pair<std::string, std::string>("rm",        "audio/x-pn-realaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("rmi",       "audio/mid"));
  mimeTypes.insert(std::pair<std::string, std::string>("rmm",       "audio/x-pn-realaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("rmp",       "audio/x-pn-realaudio"));
  mimeTypes.insert(std::pair<std::string, std::string>("rng",       "application/ringing-tones"));
  mimeTypes.insert(std::pair<std::string, std::string>("rnx",       "application/vnd.rn-realplayer"));
  mimeTypes.insert(std::pair<std::string, std::string>("roff",      "application/x-troff"));
  mimeTypes.insert(std::pair<std::string, std::string>("rp",        "image/vnd.rn-realpix"));
  mimeTypes.insert(std::pair<std::string, std::string>("rpm",       "audio/x-pn-realaudio-plugin"));
  mimeTypes.insert(std::pair<std::string, std::string>("rt",        "text/richtext"));
  mimeTypes.insert(std::pair<std::string, std::string>("rtf",       "text/richtext"));
  mimeTypes.insert(std::pair<std::string, std::string>("rtx",       "text/richtext"));
  mimeTypes.insert(std::pair<std::string, std::string>("rv",        "video/vnd.rn-realvideo"));
  mimeTypes.insert(std::pair<std::string, std::string>("s",         "text/x-asm"));
  mimeTypes.insert(std::pair<std::string, std::string>("s3m",       "audio/s3m"));
  mimeTypes.insert(std::pair<std::string, std::string>("saveme",    "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("sbk",       "application/x-tbook"));
  mimeTypes.insert(std::pair<std::string, std::string>("scm",       "video/x-scm"));
  mimeTypes.insert(std::pair<std::string, std::string>("sdml",      "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("sdp",       "application/sdp"));
  mimeTypes.insert(std::pair<std::string, std::string>("sdr",       "application/sounder"));
  mimeTypes.insert(std::pair<std::string, std::string>("sea",       "application/sea"));
  mimeTypes.insert(std::pair<std::string, std::string>("set",       "application/set"));
  mimeTypes.insert(std::pair<std::string, std::string>("sgm",       "text/sgml"));
  mimeTypes.insert(std::pair<std::string, std::string>("sgml",      "text/sgml"));
  mimeTypes.insert(std::pair<std::string, std::string>("sh",        "text/x-script.sh"));
  mimeTypes.insert(std::pair<std::string, std::string>("shar",      "application/x-bsh"));
  mimeTypes.insert(std::pair<std::string, std::string>("shtml",     "text/html"));
  mimeTypes.insert(std::pair<std::string, std::string>("shtml",     "text/x-server-parsed-html"));
  mimeTypes.insert(std::pair<std::string, std::string>("sid",       "audio/x-psid"));
  mimeTypes.insert(std::pair<std::string, std::string>("sit",       "application/x-sit"));
  mimeTypes.insert(std::pair<std::string, std::string>("sit",       "application/x-stuffit"));
  mimeTypes.insert(std::pair<std::string, std::string>("skd",       "application/x-koan"));
  mimeTypes.insert(std::pair<std::string, std::string>("skm",       "application/x-koan"));
  mimeTypes.insert(std::pair<std::string, std::string>("skp",       "application/x-koan"));
  mimeTypes.insert(std::pair<std::string, std::string>("skt",       "application/x-koan"));
  mimeTypes.insert(std::pair<std::string, std::string>("sl",        "application/x-seelogo"));
  mimeTypes.insert(std::pair<std::string, std::string>("smi",       "application/smil"));
  mimeTypes.insert(std::pair<std::string, std::string>("smil",      "application/smil"));
  mimeTypes.insert(std::pair<std::string, std::string>("snd",       "audio/basic"));
  mimeTypes.insert(std::pair<std::string, std::string>("sol",       "application/solids"));
  mimeTypes.insert(std::pair<std::string, std::string>("spc",       "text/x-speech"));
  mimeTypes.insert(std::pair<std::string, std::string>("spl",       "application/futuresplash"));
  mimeTypes.insert(std::pair<std::string, std::string>("spr",       "application/x-sprite"));
  mimeTypes.insert(std::pair<std::string, std::string>("sprite",    "application/x-sprite"));
  mimeTypes.insert(std::pair<std::string, std::string>("src",       "application/x-wais-source"));
  mimeTypes.insert(std::pair<std::string, std::string>("ssi",       "text/x-server-parsed-html"));
  mimeTypes.insert(std::pair<std::string, std::string>("ssm",       "application/streamingmedia"));
  mimeTypes.insert(std::pair<std::string, std::string>("sst",       "application/vnd.ms-pki.certstore"));
  mimeTypes.insert(std::pair<std::string, std::string>("step",      "application/step"));
  mimeTypes.insert(std::pair<std::string, std::string>("stl",       "application/sla"));
  mimeTypes.insert(std::pair<std::string, std::string>("stp",       "application/step"));
  mimeTypes.insert(std::pair<std::string, std::string>("sup",       "application/x-pgs"));
  mimeTypes.insert(std::pair<std::string, std::string>("sv4cpio",   "application/x-sv4cpio"));
  mimeTypes.insert(std::pair<std::string, std::string>("sv4crc",    "application/x-sv4crc"));
  mimeTypes.insert(std::pair<std::string, std::string>("svf",       "image/vnd.dwg"));
  mimeTypes.insert(std::pair<std::string, std::string>("svr",       "application/x-world"));
  mimeTypes.insert(std::pair<std::string, std::string>("swf",       "application/x-shockwave-flash"));
  mimeTypes.insert(std::pair<std::string, std::string>("t",         "application/x-troff"));
  mimeTypes.insert(std::pair<std::string, std::string>("talk",      "text/x-speech"));
  mimeTypes.insert(std::pair<std::string, std::string>("tar",       "application/x-tar"));
  mimeTypes.insert(std::pair<std::string, std::string>("tbk",       "application/toolbook"));
  mimeTypes.insert(std::pair<std::string, std::string>("tcl",       "text/x-script.tcl"));
  mimeTypes.insert(std::pair<std::string, std::string>("tcsh",      "text/x-script.tcsh"));
  mimeTypes.insert(std::pair<std::string, std::string>("tex",       "application/x-tex"));
  mimeTypes.insert(std::pair<std::string, std::string>("texi",      "application/x-texinfo"));
  mimeTypes.insert(std::pair<std::string, std::string>("texinfo",   "application/x-texinfo"));
  mimeTypes.insert(std::pair<std::string, std::string>("text",      "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("tgz",       "application/x-compressed"));
  mimeTypes.insert(std::pair<std::string, std::string>("tif",       "image/tiff"));
  mimeTypes.insert(std::pair<std::string, std::string>("tiff",      "image/tiff"));
  mimeTypes.insert(std::pair<std::string, std::string>("tr",        "application/x-troff"));
  mimeTypes.insert(std::pair<std::string, std::string>("ts",        "video/mp2t"));
  mimeTypes.insert(std::pair<std::string, std::string>("tsi",       "audio/tsp-audio"));
  mimeTypes.insert(std::pair<std::string, std::string>("tsp",       "audio/tsplayer"));
  mimeTypes.insert(std::pair<std::string, std::string>("tsv",       "text/tab-separated-values"));
  mimeTypes.insert(std::pair<std::string, std::string>("turbot",    "image/florian"));
  mimeTypes.insert(std::pair<std::string, std::string>("txt",       "text/plain"));
  mimeTypes.insert(std::pair<std::string, std::string>("uil",       "text/x-uil"));
  mimeTypes.insert(std::pair<std::string, std::string>("uni",       "text/uri-list"));
  mimeTypes.insert(std::pair<std::string, std::string>("unis",      "text/uri-list"));
  mimeTypes.insert(std::pair<std::string, std::string>("unv",       "application/i-deas"));
  mimeTypes.insert(std::pair<std::string, std::string>("uri",       "text/uri-list"));
  mimeTypes.insert(std::pair<std::string, std::string>("uris",      "text/uri-list"));
  mimeTypes.insert(std::pair<std::string, std::string>("ustar",     "application/x-ustar"));
  mimeTypes.insert(std::pair<std::string, std::string>("uu",        "text/x-uuencode"));
  mimeTypes.insert(std::pair<std::string, std::string>("uue",       "text/x-uuencode"));
  mimeTypes.insert(std::pair<std::string, std::string>("vcd",       "application/x-cdlink"));
  mimeTypes.insert(std::pair<std::string, std::string>("vcs",       "text/x-vcalendar"));
  mimeTypes.insert(std::pair<std::string, std::string>("vda",       "application/vda"));
  mimeTypes.insert(std::pair<std::string, std::string>("vdo",       "video/vdo"));
  mimeTypes.insert(std::pair<std::string, std::string>("vew",       "application/groupwise"));
  mimeTypes.insert(std::pair<std::string, std::string>("viv",       "video/vivo"));
  mimeTypes.insert(std::pair<std::string, std::string>("vivo",      "video/vivo"));
  mimeTypes.insert(std::pair<std::string, std::string>("vmd",       "application/vocaltec-media-desc"));
  mimeTypes.insert(std::pair<std::string, std::string>("vmf",       "application/vocaltec-media-file"));
  mimeTypes.insert(std::pair<std::string, std::string>("voc",       "audio/voc"));
  mimeTypes.insert(std::pair<std::string, std::string>("vos",       "video/vosaic"));
  mimeTypes.insert(std::pair<std::string, std::string>("vox",       "audio/voxware"));
  mimeTypes.insert(std::pair<std::string, std::string>("vqe",       "audio/x-twinvq-plugin"));
  mimeTypes.insert(std::pair<std::string, std::string>("vqf",       "audio/x-twinvq"));
  mimeTypes.insert(std::pair<std::string, std::string>("vql",       "audio/x-twinvq-plugin"));
  mimeTypes.insert(std::pair<std::string, std::string>("vrml",      "application/x-vrml"));
  mimeTypes.insert(std::pair<std::string, std::string>("vrt",       "x-world/x-vrt"));
  mimeTypes.insert(std::pair<std::string, std::string>("vsd",       "application/x-visio"));
  mimeTypes.insert(std::pair<std::string, std::string>("vst",       "application/x-visio"));
  mimeTypes.insert(std::pair<std::string, std::string>("vsw",       "application/x-visio"));
  mimeTypes.insert(std::pair<std::string, std::string>("w60",       "application/wordperfect6.0"));
  mimeTypes.insert(std::pair<std::string, std::string>("w61",       "application/wordperfect6.1"));
  mimeTypes.insert(std::pair<std::string, std::string>("w6w",       "application/msword"));
  mimeTypes.insert(std::pair<std::string, std::string>("wav",       "audio/wav"));
  mimeTypes.insert(std::pair<std::string, std::string>("wb1",       "application/x-qpro"));
  mimeTypes.insert(std::pair<std::string, std::string>("wbmp",      "image/vnd.wap.wbmp"));
  mimeTypes.insert(std::pair<std::string, std::string>("web",       "application/vnd.xara"));
  mimeTypes.insert(std::pair<std::string, std::string>("webp",      "image/webp"));
  mimeTypes.insert(std::pair<std::string, std::string>("wiz",       "application/msword"));
  mimeTypes.insert(std::pair<std::string, std::string>("wk1",       "application/x-123"));
  mimeTypes.insert(std::pair<std::string, std::string>("wma",       "audio/x-ms-wma"));
  mimeTypes.insert(std::pair<std::string, std::string>("wmf",       "windows/metafile"));
  mimeTypes.insert(std::pair<std::string, std::string>("wml",       "text/vnd.wap.wml"));
  mimeTypes.insert(std::pair<std::string, std::string>("wmlc",      "application/vnd.wap.wmlc"));
  mimeTypes.insert(std::pair<std::string, std::string>("wmls",      "text/vnd.wap.wmlscript"));
  mimeTypes.insert(std::pair<std::string, std::string>("wmlsc",     "application/vnd.wap.wmlscriptc"));
  mimeTypes.insert(std::pair<std::string, std::string>("wmv",       "video/x-ms-wmv"));
  mimeTypes.insert(std::pair<std::string, std::string>("word",      "application/msword"));
  mimeTypes.insert(std::pair<std::string, std::string>("wp",        "application/wordperfect"));
  mimeTypes.insert(std::pair<std::string, std::string>("wp5",       "application/wordperfect"));
  mimeTypes.insert(std::pair<std::string, std::string>("wp6",       "application/wordperfect"));
  mimeTypes.insert(std::pair<std::string, std::string>("wpd",       "application/wordperfect"));
  mimeTypes.insert(std::pair<std::string, std::string>("wq1",       "application/x-lotus"));
  mimeTypes.insert(std::pair<std::string, std::string>("wri",       "application/mswrite"));
  mimeTypes.insert(std::pair<std::string, std::string>("wrl",       "model/vrml"));
  mimeTypes.insert(std::pair<std::string, std::string>("wrz",       "model/vrml"));
  mimeTypes.insert(std::pair<std::string, std::string>("wsc",       "text/scriplet"));
  mimeTypes.insert(std::pair<std::string, std::string>("wsrc",      "application/x-wais-source"));
  mimeTypes.insert(std::pair<std::string, std::string>("wtk",       "application/x-wintalk"));
  mimeTypes.insert(std::pair<std::string, std::string>("xbm",       "image/xbm"));
  mimeTypes.insert(std::pair<std::string, std::string>("xdr",       "video/x-amt-demorun"));
  mimeTypes.insert(std::pair<std::string, std::string>("xgz",       "xgl/drawing"));
  mimeTypes.insert(std::pair<std::string, std::string>("xif",       "image/vnd.xiff"));
  mimeTypes.insert(std::pair<std::string, std::string>("xl",        "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xla",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlb",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlc",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xld",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlk",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xll",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlm",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xls",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlsx",      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlt",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlv",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xlw",       "application/excel"));
  mimeTypes.insert(std::pair<std::string, std::string>("xm",        "audio/xm"));
  mimeTypes.insert(std::pair<std::string, std::string>("xml",       "text/xml"));
  mimeTypes.insert(std::pair<std::string, std::string>("xmz",       "xgl/movie"));
  mimeTypes.insert(std::pair<std::string, std::string>("xpix",      "application/x-vnd.ls-xpix"));
  mimeTypes.insert(std::pair<std::string, std::string>("xpm",       "image/xpm"));
  mimeTypes.insert(std::pair<std::string, std::string>("x-png",     "image/png"));
  mimeTypes.insert(std::pair<std::string, std::string>("xsr",       "video/x-amt-showrun"));
  mimeTypes.insert(std::pair<std::string, std::string>("xvid",      "video/x-msvideo"));
  mimeTypes.insert(std::pair<std::string, std::string>("xwd",       "image/x-xwd"));
  mimeTypes.insert(std::pair<std::string, std::string>("xyz",       "chemical/x-pdb"));
  mimeTypes.insert(std::pair<std::string, std::string>("z",         "application/x-compressed"));
  mimeTypes.insert(std::pair<std::string, std::string>("zip",       "application/zip"));
  mimeTypes.insert(std::pair<std::string, std::string>("zoo",       "application/octet-stream"));
  mimeTypes.insert(std::pair<std::string, std::string>("zsh",       "text/x-script.zsh"));

  return mimeTypes;
}

std::map<std::string, std::string> CMime::m_mimetypes = fillMimeTypes();

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
  std::string path = item.GetPath();
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

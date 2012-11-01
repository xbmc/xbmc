/*
 *      Copyright (C) 2012 Team XBMC
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

#include <algorithm>

#include "Mime.h"
#include "FileItem.h"
#include "StdString.h"
#include "URIUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "video/VideoInfoTag.h"

using namespace std;

map<string, string> fillMimeTypes()
{
  map<string, string> mimeTypes;

  mimeTypes.insert(pair<string, string>("3dm",       "x-world/x-3dmf"));
  mimeTypes.insert(pair<string, string>("3dmf",      "x-world/x-3dmf"));
  mimeTypes.insert(pair<string, string>("a",         "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("aab",       "application/x-authorware-bin"));
  mimeTypes.insert(pair<string, string>("aam",       "application/x-authorware-map"));
  mimeTypes.insert(pair<string, string>("aas",       "application/x-authorware-seg"));
  mimeTypes.insert(pair<string, string>("abc",       "text/vnd.abc"));
  mimeTypes.insert(pair<string, string>("acgi",      "text/html"));
  mimeTypes.insert(pair<string, string>("afl",       "video/animaflex"));
  mimeTypes.insert(pair<string, string>("ai",        "application/postscript"));
  mimeTypes.insert(pair<string, string>("aif",       "audio/aiff"));
  mimeTypes.insert(pair<string, string>("aifc",      "audio/x-aiff"));
  mimeTypes.insert(pair<string, string>("aiff",      "audio/aiff"));
  mimeTypes.insert(pair<string, string>("aim",       "application/x-aim"));
  mimeTypes.insert(pair<string, string>("aip",       "text/x-audiosoft-intra"));
  mimeTypes.insert(pair<string, string>("ani",       "application/x-navi-animation"));
  mimeTypes.insert(pair<string, string>("aos",       "application/x-nokia-9000-communicator-add-on-software"));
  mimeTypes.insert(pair<string, string>("aps",       "application/mime"));
  mimeTypes.insert(pair<string, string>("arc",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("arj",       "application/arj"));
  mimeTypes.insert(pair<string, string>("art",       "image/x-jg"));
  mimeTypes.insert(pair<string, string>("asf",       "video/x-ms-asf"));
  mimeTypes.insert(pair<string, string>("asm",       "text/x-asm"));
  mimeTypes.insert(pair<string, string>("asp",       "text/asp"));
  mimeTypes.insert(pair<string, string>("asx",       "video/x-ms-asf"));
  mimeTypes.insert(pair<string, string>("au",        "audio/basic"));
  mimeTypes.insert(pair<string, string>("avi",       "video/x-msvideo"));
  mimeTypes.insert(pair<string, string>("avs",       "video/avs-video"));
  mimeTypes.insert(pair<string, string>("bcpio",     "application/x-bcpio"));
  mimeTypes.insert(pair<string, string>("bin",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("bm",        "image/bmp"));
  mimeTypes.insert(pair<string, string>("bmp",       "image/bmp"));
  mimeTypes.insert(pair<string, string>("boo",       "application/book"));
  mimeTypes.insert(pair<string, string>("book",      "application/book"));
  mimeTypes.insert(pair<string, string>("boz",       "application/x-bzip2"));
  mimeTypes.insert(pair<string, string>("bsh",       "application/x-bsh"));
  mimeTypes.insert(pair<string, string>("bz",        "application/x-bzip"));
  mimeTypes.insert(pair<string, string>("bz2",       "application/x-bzip2"));
  mimeTypes.insert(pair<string, string>("c",         "text/plain"));
  mimeTypes.insert(pair<string, string>("c++",       "text/plain"));
  mimeTypes.insert(pair<string, string>("cat",       "application/vnd.ms-pki.seccat"));
  mimeTypes.insert(pair<string, string>("cc",        "text/plain"));
  mimeTypes.insert(pair<string, string>("ccad",      "application/clariscad"));
  mimeTypes.insert(pair<string, string>("cco",       "application/x-cocoa"));
  mimeTypes.insert(pair<string, string>("cdf",       "application/cdf"));
  mimeTypes.insert(pair<string, string>("cer",       "application/pkix-cert"));
  mimeTypes.insert(pair<string, string>("cer",       "application/x-x509-ca-cert"));
  mimeTypes.insert(pair<string, string>("cha",       "application/x-chat"));
  mimeTypes.insert(pair<string, string>("chat",      "application/x-chat"));
  mimeTypes.insert(pair<string, string>("class",     "application/java"));
  mimeTypes.insert(pair<string, string>("com",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("conf",      "text/plain"));
  mimeTypes.insert(pair<string, string>("cpio",      "application/x-cpio"));
  mimeTypes.insert(pair<string, string>("cpp",       "text/x-c"));
  mimeTypes.insert(pair<string, string>("cpt",       "application/x-cpt"));
  mimeTypes.insert(pair<string, string>("crl",       "application/pkcs-crl"));
  mimeTypes.insert(pair<string, string>("crt",       "application/pkix-cert"));
  mimeTypes.insert(pair<string, string>("csh",       "application/x-csh"));
  mimeTypes.insert(pair<string, string>("css",       "text/css"));
  mimeTypes.insert(pair<string, string>("cxx",       "text/plain"));
  mimeTypes.insert(pair<string, string>("dcr",       "application/x-director"));
  mimeTypes.insert(pair<string, string>("deepv",     "application/x-deepv"));
  mimeTypes.insert(pair<string, string>("def",       "text/plain"));
  mimeTypes.insert(pair<string, string>("der",       "application/x-x509-ca-cert"));
  mimeTypes.insert(pair<string, string>("dif",       "video/x-dv"));
  mimeTypes.insert(pair<string, string>("dir",       "application/x-director"));
  mimeTypes.insert(pair<string, string>("dl",        "video/dl"));
  mimeTypes.insert(pair<string, string>("doc",       "application/msword"));
  mimeTypes.insert(pair<string, string>("docx",      "application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
  mimeTypes.insert(pair<string, string>("dot",       "application/msword"));
  mimeTypes.insert(pair<string, string>("dp",        "application/commonground"));
  mimeTypes.insert(pair<string, string>("drw",       "application/drafting"));
  mimeTypes.insert(pair<string, string>("dump",      "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("dv",        "video/x-dv"));
  mimeTypes.insert(pair<string, string>("dvi",       "application/x-dvi"));
  mimeTypes.insert(pair<string, string>("dwf",       "model/vnd.dwf"));
  mimeTypes.insert(pair<string, string>("dwg",       "image/vnd.dwg"));
  mimeTypes.insert(pair<string, string>("dxf",       "image/vnd.dwg"));
  mimeTypes.insert(pair<string, string>("dxr",       "application/x-director"));
  mimeTypes.insert(pair<string, string>("el",        "text/x-script.elisp"));
  mimeTypes.insert(pair<string, string>("elc",       "application/x-elc"));
  mimeTypes.insert(pair<string, string>("env",       "application/x-envoy"));
  mimeTypes.insert(pair<string, string>("eps",       "application/postscript"));
  mimeTypes.insert(pair<string, string>("es",        "application/x-esrehber"));
  mimeTypes.insert(pair<string, string>("etx",       "text/x-setext"));
  mimeTypes.insert(pair<string, string>("evy",       "application/envoy"));
  mimeTypes.insert(pair<string, string>("exe",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("f",         "text/x-fortran"));
  mimeTypes.insert(pair<string, string>("f77",       "text/x-fortran"));
  mimeTypes.insert(pair<string, string>("f90",       "text/x-fortran"));
  mimeTypes.insert(pair<string, string>("fdf",       "application/vnd.fdf"));
  mimeTypes.insert(pair<string, string>("fif",       "image/fif"));
  mimeTypes.insert(pair<string, string>("flac",      "audio/flac"));
  mimeTypes.insert(pair<string, string>("fli",       "video/fli"));
  mimeTypes.insert(pair<string, string>("flo",       "image/florian"));
  mimeTypes.insert(pair<string, string>("flv",       "video/x-flv"));
  mimeTypes.insert(pair<string, string>("flx",       "text/vnd.fmi.flexstor"));
  mimeTypes.insert(pair<string, string>("fmf",       "video/x-atomic3d-feature"));
  mimeTypes.insert(pair<string, string>("for",       "text/plain"));
  mimeTypes.insert(pair<string, string>("for",       "text/x-fortran"));
  mimeTypes.insert(pair<string, string>("fpx",       "image/vnd.fpx"));
  mimeTypes.insert(pair<string, string>("frl",       "application/freeloader"));
  mimeTypes.insert(pair<string, string>("funk",      "audio/make"));
  mimeTypes.insert(pair<string, string>("g",         "text/plain"));
  mimeTypes.insert(pair<string, string>("g3",        "image/g3fax"));
  mimeTypes.insert(pair<string, string>("gif",       "image/gif"));
  mimeTypes.insert(pair<string, string>("gl",        "video/x-gl"));
  mimeTypes.insert(pair<string, string>("gsd",       "audio/x-gsm"));
  mimeTypes.insert(pair<string, string>("gsm",       "audio/x-gsm"));
  mimeTypes.insert(pair<string, string>("gsp",       "application/x-gsp"));
  mimeTypes.insert(pair<string, string>("gss",       "application/x-gss"));
  mimeTypes.insert(pair<string, string>("gtar",      "application/x-gtar"));
  mimeTypes.insert(pair<string, string>("gz",        "application/x-compressed"));
  mimeTypes.insert(pair<string, string>("gzip",      "application/x-gzip"));
  mimeTypes.insert(pair<string, string>("h",         "text/plain"));
  mimeTypes.insert(pair<string, string>("hdf",       "application/x-hdf"));
  mimeTypes.insert(pair<string, string>("help",      "application/x-helpfile"));
  mimeTypes.insert(pair<string, string>("hgl",       "application/vnd.hp-hpgl"));
  mimeTypes.insert(pair<string, string>("hh",        "text/plain"));
  mimeTypes.insert(pair<string, string>("hlb",       "text/x-script"));
  mimeTypes.insert(pair<string, string>("hlp",       "application/hlp"));
  mimeTypes.insert(pair<string, string>("hpg",       "application/vnd.hp-hpgl"));
  mimeTypes.insert(pair<string, string>("hpgl",      "application/vnd.hp-hpgl"));
  mimeTypes.insert(pair<string, string>("hqx",       "application/binhex"));
  mimeTypes.insert(pair<string, string>("hta",       "application/hta"));
  mimeTypes.insert(pair<string, string>("htc",       "text/x-component"));
  mimeTypes.insert(pair<string, string>("htm",       "text/html"));
  mimeTypes.insert(pair<string, string>("html",      "text/html"));
  mimeTypes.insert(pair<string, string>("htmls",     "text/html"));
  mimeTypes.insert(pair<string, string>("htt",       "text/webviewhtml"));
  mimeTypes.insert(pair<string, string>("htx",       "text/html"));
  mimeTypes.insert(pair<string, string>("ice",       "x-conference/x-cooltalk"));
  mimeTypes.insert(pair<string, string>("ico",       "image/x-icon"));
  mimeTypes.insert(pair<string, string>("idc",       "text/plain"));
  mimeTypes.insert(pair<string, string>("ief",       "image/ief"));
  mimeTypes.insert(pair<string, string>("iefs",      "image/ief"));
  mimeTypes.insert(pair<string, string>("iges",      "application/iges"));
  mimeTypes.insert(pair<string, string>("igs",       "application/iges"));
  mimeTypes.insert(pair<string, string>("igs",       "model/iges"));
  mimeTypes.insert(pair<string, string>("ima",       "application/x-ima"));
  mimeTypes.insert(pair<string, string>("imap",      "application/x-httpd-imap"));
  mimeTypes.insert(pair<string, string>("inf",       "application/inf"));
  mimeTypes.insert(pair<string, string>("ins",       "application/x-internett-signup"));
  mimeTypes.insert(pair<string, string>("ip",        "application/x-ip2"));
  mimeTypes.insert(pair<string, string>("isu",       "video/x-isvideo"));
  mimeTypes.insert(pair<string, string>("it",        "audio/it"));
  mimeTypes.insert(pair<string, string>("iv",        "application/x-inventor"));
  mimeTypes.insert(pair<string, string>("ivr",       "i-world/i-vrml"));
  mimeTypes.insert(pair<string, string>("ivy",       "application/x-livescreen"));
  mimeTypes.insert(pair<string, string>("jam",       "audio/x-jam"));
  mimeTypes.insert(pair<string, string>("jav",       "text/x-java-source"));
  mimeTypes.insert(pair<string, string>("java",      "text/x-java-source"));
  mimeTypes.insert(pair<string, string>("jcm",       "application/x-java-commerce"));
  mimeTypes.insert(pair<string, string>("jfif",      "image/jpeg"));
  mimeTypes.insert(pair<string, string>("jfif-tbnl", "image/jpeg"));
  mimeTypes.insert(pair<string, string>("jpe",       "image/jpeg"));
  mimeTypes.insert(pair<string, string>("jpeg",      "image/jpeg"));
  mimeTypes.insert(pair<string, string>("jpg",       "image/jpeg"));
  mimeTypes.insert(pair<string, string>("jps",       "image/x-jps"));
  mimeTypes.insert(pair<string, string>("js",        "application/x-javascript"));
  mimeTypes.insert(pair<string, string>("jut",       "image/jutvision"));
  mimeTypes.insert(pair<string, string>("kar",       "music/x-karaoke"));
  mimeTypes.insert(pair<string, string>("ksh",       "application/x-ksh"));
  mimeTypes.insert(pair<string, string>("ksh",       "text/x-script.ksh"));
  mimeTypes.insert(pair<string, string>("la",        "audio/nspaudio"));
  mimeTypes.insert(pair<string, string>("lam",       "audio/x-liveaudio"));
  mimeTypes.insert(pair<string, string>("latex",     "application/x-latex"));
  mimeTypes.insert(pair<string, string>("lha",       "application/lha"));
  mimeTypes.insert(pair<string, string>("lhx",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("list",      "text/plain"));
  mimeTypes.insert(pair<string, string>("lma",       "audio/nspaudio"));
  mimeTypes.insert(pair<string, string>("log",       "text/plain"));
  mimeTypes.insert(pair<string, string>("lsp",       "application/x-lisp"));
  mimeTypes.insert(pair<string, string>("lst",       "text/plain"));
  mimeTypes.insert(pair<string, string>("lsx",       "text/x-la-asf"));
  mimeTypes.insert(pair<string, string>("ltx",       "application/x-latex"));
  mimeTypes.insert(pair<string, string>("lzh",       "application/x-lzh"));
  mimeTypes.insert(pair<string, string>("lzx",       "application/lzx"));
  mimeTypes.insert(pair<string, string>("m",         "text/x-m"));
  mimeTypes.insert(pair<string, string>("m1v",       "video/mpeg"));
  mimeTypes.insert(pair<string, string>("m2a",       "audio/mpeg"));
  mimeTypes.insert(pair<string, string>("m2v",       "video/mpeg"));
  mimeTypes.insert(pair<string, string>("m3u",       "audio/x-mpequrl"));
  mimeTypes.insert(pair<string, string>("man",       "application/x-troff-man"));
  mimeTypes.insert(pair<string, string>("map",       "application/x-navimap"));
  mimeTypes.insert(pair<string, string>("mar",       "text/plain"));
  mimeTypes.insert(pair<string, string>("mbd",       "application/mbedlet"));
  mimeTypes.insert(pair<string, string>("mc$",       "application/x-magic-cap-package-1.0"));
  mimeTypes.insert(pair<string, string>("mcd",       "application/x-mathcad"));
  mimeTypes.insert(pair<string, string>("mcf",       "text/mcf"));
  mimeTypes.insert(pair<string, string>("mcp",       "application/netmc"));
  mimeTypes.insert(pair<string, string>("me",        "application/x-troff-me"));
  mimeTypes.insert(pair<string, string>("mht",       "message/rfc822"));
  mimeTypes.insert(pair<string, string>("mhtml",     "message/rfc822"));
  mimeTypes.insert(pair<string, string>("mid",       "audio/midi"));
  mimeTypes.insert(pair<string, string>("midi",      "audio/midi"));
  mimeTypes.insert(pair<string, string>("mif",       "application/x-mif"));
  mimeTypes.insert(pair<string, string>("mime",      "message/rfc822"));
  mimeTypes.insert(pair<string, string>("mjf",       "audio/x-vnd.audioexplosion.mjuicemediafile"));
  mimeTypes.insert(pair<string, string>("mjpg",      "video/x-motion-jpeg"));
  mimeTypes.insert(pair<string, string>("mka",       "audio/x-matroska"));
  mimeTypes.insert(pair<string, string>("mkv",       "video/x-matroska"));
  mimeTypes.insert(pair<string, string>("mm",        "application/x-meme"));
  mimeTypes.insert(pair<string, string>("mme",       "application/base64"));
  mimeTypes.insert(pair<string, string>("mod",       "audio/mod"));
  mimeTypes.insert(pair<string, string>("moov",      "video/quicktime"));
  mimeTypes.insert(pair<string, string>("mov",       "video/quicktime"));
  mimeTypes.insert(pair<string, string>("movie",     "video/x-sgi-movie"));
  mimeTypes.insert(pair<string, string>("mp2",       "audio/mpeg"));
  mimeTypes.insert(pair<string, string>("mp3",       "audio/mpeg3"));
  mimeTypes.insert(pair<string, string>("mp4",       "video/mp4"));
  mimeTypes.insert(pair<string, string>("mpa",       "audio/mpeg"));
  mimeTypes.insert(pair<string, string>("mpc",       "application/x-project"));
  mimeTypes.insert(pair<string, string>("mpe",       "video/mpeg"));
  mimeTypes.insert(pair<string, string>("mpeg",      "video/mpeg"));
  mimeTypes.insert(pair<string, string>("mpg",       "video/mpeg"));
  mimeTypes.insert(pair<string, string>("mpga",      "audio/mpeg"));
  mimeTypes.insert(pair<string, string>("mpp",       "application/vnd.ms-project"));
  mimeTypes.insert(pair<string, string>("mpt",       "application/x-project"));
  mimeTypes.insert(pair<string, string>("mpv",       "application/x-project"));
  mimeTypes.insert(pair<string, string>("mpx",       "application/x-project"));
  mimeTypes.insert(pair<string, string>("mrc",       "application/marc"));
  mimeTypes.insert(pair<string, string>("ms",        "application/x-troff-ms"));
  mimeTypes.insert(pair<string, string>("mv",        "video/x-sgi-movie"));
  mimeTypes.insert(pair<string, string>("my",        "audio/make"));
  mimeTypes.insert(pair<string, string>("mzz",       "application/x-vnd.audioexplosion.mzz"));
  mimeTypes.insert(pair<string, string>("nap",       "image/naplps"));
  mimeTypes.insert(pair<string, string>("naplps",    "image/naplps"));
  mimeTypes.insert(pair<string, string>("nc",        "application/x-netcdf"));
  mimeTypes.insert(pair<string, string>("ncm",       "application/vnd.nokia.configuration-message"));
  mimeTypes.insert(pair<string, string>("nfo",       "text/xml"));
  mimeTypes.insert(pair<string, string>("nif",       "image/x-niff"));
  mimeTypes.insert(pair<string, string>("niff",      "image/x-niff"));
  mimeTypes.insert(pair<string, string>("nix",       "application/x-mix-transfer"));
  mimeTypes.insert(pair<string, string>("nsc",       "application/x-conference"));
  mimeTypes.insert(pair<string, string>("nvd",       "application/x-navidoc"));
  mimeTypes.insert(pair<string, string>("o",         "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("oda",       "application/oda"));
  mimeTypes.insert(pair<string, string>("ogg",       "audio/ogg"));
  mimeTypes.insert(pair<string, string>("omc",       "application/x-omc"));
  mimeTypes.insert(pair<string, string>("omcd",      "application/x-omcdatamaker"));
  mimeTypes.insert(pair<string, string>("omcr",      "application/x-omcregerator"));
  mimeTypes.insert(pair<string, string>("p",         "text/x-pascal"));
  mimeTypes.insert(pair<string, string>("p10",       "application/pkcs10"));
  mimeTypes.insert(pair<string, string>("p12",       "application/pkcs-12"));
  mimeTypes.insert(pair<string, string>("p7a",       "application/x-pkcs7-signature"));
  mimeTypes.insert(pair<string, string>("p7c",       "application/pkcs7-mime"));
  mimeTypes.insert(pair<string, string>("p7m",       "application/pkcs7-mime"));
  mimeTypes.insert(pair<string, string>("p7r",       "application/x-pkcs7-certreqresp"));
  mimeTypes.insert(pair<string, string>("p7s",       "application/pkcs7-signature"));
  mimeTypes.insert(pair<string, string>("part",      "application/pro_eng"));
  mimeTypes.insert(pair<string, string>("pas",       "text/pascal"));
  mimeTypes.insert(pair<string, string>("pbm",       "image/x-portable-bitmap"));
  mimeTypes.insert(pair<string, string>("pcl",       "application/vnd.hp-pcl"));
  mimeTypes.insert(pair<string, string>("pct",       "image/x-pict"));
  mimeTypes.insert(pair<string, string>("pcx",       "image/x-pcx"));
  mimeTypes.insert(pair<string, string>("pdb",       "chemical/x-pdb"));
  mimeTypes.insert(pair<string, string>("pdf",       "application/pdf"));
  mimeTypes.insert(pair<string, string>("pfunk",     "audio/make.my.funk"));
  mimeTypes.insert(pair<string, string>("pgm",       "image/x-portable-greymap"));
  mimeTypes.insert(pair<string, string>("pic",       "image/pict"));
  mimeTypes.insert(pair<string, string>("pict",      "image/pict"));
  mimeTypes.insert(pair<string, string>("pkg",       "application/x-newton-compatible-pkg"));
  mimeTypes.insert(pair<string, string>("pko",       "application/vnd.ms-pki.pko"));
  mimeTypes.insert(pair<string, string>("pl",        "text/x-script.perl"));
  mimeTypes.insert(pair<string, string>("plx",       "application/x-pixclscript"));
  mimeTypes.insert(pair<string, string>("pm",        "text/x-script.perl-module"));
  mimeTypes.insert(pair<string, string>("pm4",       "application/x-pagemaker"));
  mimeTypes.insert(pair<string, string>("pm5",       "application/x-pagemaker"));
  mimeTypes.insert(pair<string, string>("png",       "image/png"));
  mimeTypes.insert(pair<string, string>("pnm",       "application/x-portable-anymap"));
  mimeTypes.insert(pair<string, string>("pot",       "application/vnd.ms-powerpoint"));
  mimeTypes.insert(pair<string, string>("pov",       "model/x-pov"));
  mimeTypes.insert(pair<string, string>("ppa",       "application/vnd.ms-powerpoint"));
  mimeTypes.insert(pair<string, string>("ppm",       "image/x-portable-pixmap"));
  mimeTypes.insert(pair<string, string>("pps",       "application/mspowerpoint"));
  mimeTypes.insert(pair<string, string>("ppt",       "application/mspowerpoint"));
  mimeTypes.insert(pair<string, string>("ppz",       "application/mspowerpoint"));
  mimeTypes.insert(pair<string, string>("pre",       "application/x-freelance"));
  mimeTypes.insert(pair<string, string>("prt",       "application/pro_eng"));
  mimeTypes.insert(pair<string, string>("ps",        "application/postscript"));
  mimeTypes.insert(pair<string, string>("psd",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("pvu",       "paleovu/x-pv"));
  mimeTypes.insert(pair<string, string>("pwz",       "application/vnd.ms-powerpoint"));
  mimeTypes.insert(pair<string, string>("py",        "text/x-script.phyton"));
  mimeTypes.insert(pair<string, string>("pyc",       "applicaiton/x-bytecode.python"));
  mimeTypes.insert(pair<string, string>("qcp",       "audio/vnd.qcelp"));
  mimeTypes.insert(pair<string, string>("qd3",       "x-world/x-3dmf"));
  mimeTypes.insert(pair<string, string>("qd3d",      "x-world/x-3dmf"));
  mimeTypes.insert(pair<string, string>("qif",       "image/x-quicktime"));
  mimeTypes.insert(pair<string, string>("qt",        "video/quicktime"));
  mimeTypes.insert(pair<string, string>("qtc",       "video/x-qtc"));
  mimeTypes.insert(pair<string, string>("qti",       "image/x-quicktime"));
  mimeTypes.insert(pair<string, string>("qtif",      "image/x-quicktime"));
  mimeTypes.insert(pair<string, string>("ra",        "audio/x-realaudio"));
  mimeTypes.insert(pair<string, string>("ram",       "audio/x-pn-realaudio"));
  mimeTypes.insert(pair<string, string>("ras",       "image/cmu-raster"));
  mimeTypes.insert(pair<string, string>("rast",      "image/cmu-raster"));
  mimeTypes.insert(pair<string, string>("rexx",      "text/x-script.rexx"));
  mimeTypes.insert(pair<string, string>("rf",        "image/vnd.rn-realflash"));
  mimeTypes.insert(pair<string, string>("rgb",       "image/x-rgb"));
  mimeTypes.insert(pair<string, string>("rm",        "audio/x-pn-realaudio"));
  mimeTypes.insert(pair<string, string>("rmi",       "audio/mid"));
  mimeTypes.insert(pair<string, string>("rmm",       "audio/x-pn-realaudio"));
  mimeTypes.insert(pair<string, string>("rmp",       "audio/x-pn-realaudio"));
  mimeTypes.insert(pair<string, string>("rng",       "application/ringing-tones"));
  mimeTypes.insert(pair<string, string>("rnx",       "application/vnd.rn-realplayer"));
  mimeTypes.insert(pair<string, string>("roff",      "application/x-troff"));
  mimeTypes.insert(pair<string, string>("rp",        "image/vnd.rn-realpix"));
  mimeTypes.insert(pair<string, string>("rpm",       "audio/x-pn-realaudio-plugin"));
  mimeTypes.insert(pair<string, string>("rt",        "text/richtext"));
  mimeTypes.insert(pair<string, string>("rtf",       "text/richtext"));
  mimeTypes.insert(pair<string, string>("rtx",       "text/richtext"));
  mimeTypes.insert(pair<string, string>("rv",        "video/vnd.rn-realvideo"));
  mimeTypes.insert(pair<string, string>("s",         "text/x-asm"));
  mimeTypes.insert(pair<string, string>("s3m",       "audio/s3m"));
  mimeTypes.insert(pair<string, string>("saveme",    "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("sbk",       "application/x-tbook"));
  mimeTypes.insert(pair<string, string>("scm",       "video/x-scm"));
  mimeTypes.insert(pair<string, string>("sdml",      "text/plain"));
  mimeTypes.insert(pair<string, string>("sdp",       "application/sdp"));
  mimeTypes.insert(pair<string, string>("sdr",       "application/sounder"));
  mimeTypes.insert(pair<string, string>("sea",       "application/sea"));
  mimeTypes.insert(pair<string, string>("set",       "application/set"));
  mimeTypes.insert(pair<string, string>("sgm",       "text/sgml"));
  mimeTypes.insert(pair<string, string>("sgml",      "text/sgml"));
  mimeTypes.insert(pair<string, string>("sh",        "text/x-script.sh"));
  mimeTypes.insert(pair<string, string>("shar",      "application/x-bsh"));
  mimeTypes.insert(pair<string, string>("shtml",     "text/html"));
  mimeTypes.insert(pair<string, string>("shtml",     "text/x-server-parsed-html"));
  mimeTypes.insert(pair<string, string>("sid",       "audio/x-psid"));
  mimeTypes.insert(pair<string, string>("sit",       "application/x-sit"));
  mimeTypes.insert(pair<string, string>("sit",       "application/x-stuffit"));
  mimeTypes.insert(pair<string, string>("skd",       "application/x-koan"));
  mimeTypes.insert(pair<string, string>("skm",       "application/x-koan"));
  mimeTypes.insert(pair<string, string>("skp",       "application/x-koan"));
  mimeTypes.insert(pair<string, string>("skt",       "application/x-koan"));
  mimeTypes.insert(pair<string, string>("sl",        "application/x-seelogo"));
  mimeTypes.insert(pair<string, string>("smi",       "application/smil"));
  mimeTypes.insert(pair<string, string>("smil",      "application/smil"));
  mimeTypes.insert(pair<string, string>("snd",       "audio/basic"));
  mimeTypes.insert(pair<string, string>("sol",       "application/solids"));
  mimeTypes.insert(pair<string, string>("spc",       "text/x-speech"));
  mimeTypes.insert(pair<string, string>("spl",       "application/futuresplash"));
  mimeTypes.insert(pair<string, string>("spr",       "application/x-sprite"));
  mimeTypes.insert(pair<string, string>("sprite",    "application/x-sprite"));
  mimeTypes.insert(pair<string, string>("src",       "application/x-wais-source"));
  mimeTypes.insert(pair<string, string>("ssi",       "text/x-server-parsed-html"));
  mimeTypes.insert(pair<string, string>("ssm",       "application/streamingmedia"));
  mimeTypes.insert(pair<string, string>("sst",       "application/vnd.ms-pki.certstore"));
  mimeTypes.insert(pair<string, string>("step",      "application/step"));
  mimeTypes.insert(pair<string, string>("stl",       "application/sla"));
  mimeTypes.insert(pair<string, string>("stp",       "application/step"));
  mimeTypes.insert(pair<string, string>("sv4cpio",   "application/x-sv4cpio"));
  mimeTypes.insert(pair<string, string>("sv4crc",    "application/x-sv4crc"));
  mimeTypes.insert(pair<string, string>("svf",       "image/vnd.dwg"));
  mimeTypes.insert(pair<string, string>("svr",       "application/x-world"));
  mimeTypes.insert(pair<string, string>("swf",       "application/x-shockwave-flash"));
  mimeTypes.insert(pair<string, string>("t",         "application/x-troff"));
  mimeTypes.insert(pair<string, string>("talk",      "text/x-speech"));
  mimeTypes.insert(pair<string, string>("tar",       "application/x-tar"));
  mimeTypes.insert(pair<string, string>("tbk",       "application/toolbook"));
  mimeTypes.insert(pair<string, string>("tcl",       "text/x-script.tcl"));
  mimeTypes.insert(pair<string, string>("tcsh",      "text/x-script.tcsh"));
  mimeTypes.insert(pair<string, string>("tex",       "application/x-tex"));
  mimeTypes.insert(pair<string, string>("texi",      "application/x-texinfo"));
  mimeTypes.insert(pair<string, string>("texinfo",   "application/x-texinfo"));
  mimeTypes.insert(pair<string, string>("text",      "text/plain"));
  mimeTypes.insert(pair<string, string>("tgz",       "application/x-compressed"));
  mimeTypes.insert(pair<string, string>("tif",       "image/tiff"));
  mimeTypes.insert(pair<string, string>("tr",        "application/x-troff"));
  mimeTypes.insert(pair<string, string>("ts",        "video/mp2t"));
  mimeTypes.insert(pair<string, string>("tsi",       "audio/tsp-audio"));
  mimeTypes.insert(pair<string, string>("tsp",       "audio/tsplayer"));
  mimeTypes.insert(pair<string, string>("tsv",       "text/tab-separated-values"));
  mimeTypes.insert(pair<string, string>("turbot",    "image/florian"));
  mimeTypes.insert(pair<string, string>("txt",       "text/plain"));
  mimeTypes.insert(pair<string, string>("uil",       "text/x-uil"));
  mimeTypes.insert(pair<string, string>("uni",       "text/uri-list"));
  mimeTypes.insert(pair<string, string>("unis",      "text/uri-list"));
  mimeTypes.insert(pair<string, string>("unv",       "application/i-deas"));
  mimeTypes.insert(pair<string, string>("uri",       "text/uri-list"));
  mimeTypes.insert(pair<string, string>("uris",      "text/uri-list"));
  mimeTypes.insert(pair<string, string>("ustar",     "application/x-ustar"));
  mimeTypes.insert(pair<string, string>("uu",        "text/x-uuencode"));
  mimeTypes.insert(pair<string, string>("uue",       "text/x-uuencode"));
  mimeTypes.insert(pair<string, string>("vcd",       "application/x-cdlink"));
  mimeTypes.insert(pair<string, string>("vcs",       "text/x-vcalendar"));
  mimeTypes.insert(pair<string, string>("vda",       "application/vda"));
  mimeTypes.insert(pair<string, string>("vdo",       "video/vdo"));
  mimeTypes.insert(pair<string, string>("vew",       "application/groupwise"));
  mimeTypes.insert(pair<string, string>("viv",       "video/vivo"));
  mimeTypes.insert(pair<string, string>("vivo",      "video/vivo"));
  mimeTypes.insert(pair<string, string>("vmd",       "application/vocaltec-media-desc"));
  mimeTypes.insert(pair<string, string>("vmf",       "application/vocaltec-media-file"));
  mimeTypes.insert(pair<string, string>("voc",       "audio/voc"));
  mimeTypes.insert(pair<string, string>("vos",       "video/vosaic"));
  mimeTypes.insert(pair<string, string>("vox",       "audio/voxware"));
  mimeTypes.insert(pair<string, string>("vqe",       "audio/x-twinvq-plugin"));
  mimeTypes.insert(pair<string, string>("vqf",       "audio/x-twinvq"));
  mimeTypes.insert(pair<string, string>("vql",       "audio/x-twinvq-plugin"));
  mimeTypes.insert(pair<string, string>("vrml",      "application/x-vrml"));
  mimeTypes.insert(pair<string, string>("vrt",       "x-world/x-vrt"));
  mimeTypes.insert(pair<string, string>("vsd",       "application/x-visio"));
  mimeTypes.insert(pair<string, string>("vst",       "application/x-visio"));
  mimeTypes.insert(pair<string, string>("vsw",       "application/x-visio"));
  mimeTypes.insert(pair<string, string>("w60",       "application/wordperfect6.0"));
  mimeTypes.insert(pair<string, string>("w61",       "application/wordperfect6.1"));
  mimeTypes.insert(pair<string, string>("w6w",       "application/msword"));
  mimeTypes.insert(pair<string, string>("wav",       "audio/wav"));
  mimeTypes.insert(pair<string, string>("wb1",       "application/x-qpro"));
  mimeTypes.insert(pair<string, string>("wbmp",      "image/vnd.wap.wbmp"));
  mimeTypes.insert(pair<string, string>("web",       "application/vnd.xara"));
  mimeTypes.insert(pair<string, string>("wiz",       "application/msword"));
  mimeTypes.insert(pair<string, string>("wk1",       "application/x-123"));
  mimeTypes.insert(pair<string, string>("wma",       "audio/x-ms-wma"));
  mimeTypes.insert(pair<string, string>("wmf",       "windows/metafile"));
  mimeTypes.insert(pair<string, string>("wml",       "text/vnd.wap.wml"));
  mimeTypes.insert(pair<string, string>("wmlc",      "application/vnd.wap.wmlc"));
  mimeTypes.insert(pair<string, string>("wmls",      "text/vnd.wap.wmlscript"));
  mimeTypes.insert(pair<string, string>("wmlsc",     "application/vnd.wap.wmlscriptc"));
  mimeTypes.insert(pair<string, string>("wmv",       "video/x-ms-wmv"));
  mimeTypes.insert(pair<string, string>("word",      "application/msword"));
  mimeTypes.insert(pair<string, string>("wp",        "application/wordperfect"));
  mimeTypes.insert(pair<string, string>("wp5",       "application/wordperfect"));
  mimeTypes.insert(pair<string, string>("wp6",       "application/wordperfect"));
  mimeTypes.insert(pair<string, string>("wpd",       "application/wordperfect"));
  mimeTypes.insert(pair<string, string>("wq1",       "application/x-lotus"));
  mimeTypes.insert(pair<string, string>("wri",       "application/mswrite"));
  mimeTypes.insert(pair<string, string>("wrl",       "model/vrml"));
  mimeTypes.insert(pair<string, string>("wrz",       "model/vrml"));
  mimeTypes.insert(pair<string, string>("wsc",       "text/scriplet"));
  mimeTypes.insert(pair<string, string>("wsrc",      "application/x-wais-source"));
  mimeTypes.insert(pair<string, string>("wtk",       "application/x-wintalk"));
  mimeTypes.insert(pair<string, string>("xbm",       "image/xbm"));
  mimeTypes.insert(pair<string, string>("xdr",       "video/x-amt-demorun"));
  mimeTypes.insert(pair<string, string>("xgz",       "xgl/drawing"));
  mimeTypes.insert(pair<string, string>("xif",       "image/vnd.xiff"));
  mimeTypes.insert(pair<string, string>("xl",        "application/excel"));
  mimeTypes.insert(pair<string, string>("xla",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlb",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlc",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xld",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlk",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xll",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlm",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xls",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlsx",      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
  mimeTypes.insert(pair<string, string>("xlt",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlv",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xlw",       "application/excel"));
  mimeTypes.insert(pair<string, string>("xm",        "audio/xm"));
  mimeTypes.insert(pair<string, string>("xml",       "text/xml"));
  mimeTypes.insert(pair<string, string>("xmz",       "xgl/movie"));
  mimeTypes.insert(pair<string, string>("xpix",      "application/x-vnd.ls-xpix"));
  mimeTypes.insert(pair<string, string>("xpm",       "image/xpm"));
  mimeTypes.insert(pair<string, string>("x-png",     "image/png"));
  mimeTypes.insert(pair<string, string>("xsr",       "video/x-amt-showrun"));
  mimeTypes.insert(pair<string, string>("xwd",       "image/x-xwd"));
  mimeTypes.insert(pair<string, string>("xyz",       "chemical/x-pdb"));
  mimeTypes.insert(pair<string, string>("z",         "application/x-compressed"));
  mimeTypes.insert(pair<string, string>("zip",       "application/zip"));
  mimeTypes.insert(pair<string, string>("zoo",       "application/octet-stream"));
  mimeTypes.insert(pair<string, string>("zsh",       "text/x-script.zsh"));

  return mimeTypes;
}

map<string, string> CMime::m_mimetypes = fillMimeTypes();

string CMime::GetMimeType(const string &extension)
{
  if (extension.empty())
    return "";

  string ext = extension;
  size_t posNotPoint = ext.find_first_not_of('.');
  if (posNotPoint != string::npos && posNotPoint > 0)
    ext = extension.substr(posNotPoint);
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  map<string, string>::const_iterator it = m_mimetypes.find(ext);
  if (it != m_mimetypes.end())
    return it->second;

  return "";
}

string CMime::GetMimeType(const CFileItem &item)
{
  CStdString path = item.GetPath();
  if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->GetPath().IsEmpty())
    path = item.GetVideoInfoTag()->GetPath();
  else if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->GetURL().IsEmpty())
    path = item.GetMusicInfoTag()->GetURL();

  return GetMimeType(URIUtils::GetExtension(path));
}


UnZip 0.15 additionnal library


  This unzip package allow extract file from .ZIP file, compatible with 
PKZip 2.04g, WinZip, InfoZip tools and compatible.

  Multi volume ZipFile (span) are not supported, and old compression used by old 
PKZip 1.x are not supported.

See probdesc.zip from PKWare for specification of .ZIP format.

What is Unzip
  The Zlib library support the deflate compression and the creation of gzip (.gz) 
file. Zlib is free and small.
  The .Zip format, which can contain several compressed files (.gz can containt
only one file) is a very popular format. This is why I've written a package for reading file compressed in Zipfile.

Using Unzip package

You need source of Zlib (get zlib111.zip and read zlib.h).
Get unzlb015.zip and read unzip.h (whith documentation of unzip functions)

The Unzip package is only two file : unzip.h and unzip.c. But it use the Zlib 
  files.
unztst.c is a simple sample program, which list file in a zipfile and display
  README.TXT or FILE_ID.DIZ (if these files are found).
miniunz.c is a mini unzip program.

I'm also currenlyt writing a zipping portion (zip.h, zip.c and test with minizip.c)

Please email me for feedback.
I hope my source is compatible with Unix system, but I need your help for be sure

Latest revision : Mar 04th, 1998

Check http://www.winimage.com/zLibDll/unzip.html for up to date info.

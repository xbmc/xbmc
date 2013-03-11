set(cximage_srcs
	ximadsp.cpp ximage.cpp ximaico.cpp 
	ximalpha.cpp ximapal.cpp  ximatga.cpp ximatran.cpp
	tif_xfile.cpp ximaenc.cpp ximagif.cpp ximainfo.cpp
	ximapcx.cpp ximasel.cpp ximath.cpp ximawbmp.cpp
	xmemfile.cpp ximabmp.cpp ximaexif.cpp ximahist.cpp
  ximaint.cpp ximaska.cpp
)

if(CX_SUPPORT_JSP)
  list(APPEND cximage_srcs ximajas.cpp)
endif(CX_SUPPORT_JSP)

if(CX_SUPPORT_JBIG)
  list(APPEND cximage_srcs ximajbg.cpp)
endif(CX_SUPPORT_JBIG)

if(CX_SUPPORT_MNG)
  list(APPEND cximage_srcs ximamng.cpp)
endif(CX_SUPPORT_MNG)

if(CX_SUPPORT_RAW)
  list(APPEND cximage_srcs ximaraw.cpp)
endif(CX_SUPPORT_RAW)

if(CX_SUPPORT_PNG)
  list(APPEND cximage_srcs ximapng.cpp)
endif(CX_SUPPORT_PNG)

if(CX_SUPPORT_JPG)
  list(APPEND cximage_srcs ximajpg.cpp)
endif(CX_SUPPORT_JPG)

if(CX_SUPPORT_TIFF)
  list(APPEND cximage_srcs ximatif.cpp)
endif(CX_SUPPORT_TIFF)

if(CX_SUPPORT_RAW)
  list(APPEND cximage_srcs ximaraw.cpp)
endif(CX_SUPPORT_RAW)

if(TARGET_WIN32)
  list(APPEND cximage_srcs CxImageDLL/CxImageCrtDll.cpp ximawmf.cpp)
endif(TARGET_WIN32)
include_directories(../jasper/include)
add_library(cximage STATIC ${cximage_srcs})

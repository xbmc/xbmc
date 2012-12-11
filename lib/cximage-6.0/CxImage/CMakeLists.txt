set(cximage_srcs
	DllInterface.cpp ximadsp.cpp ximage.cpp ximaico.cpp ximajas.cpp
	ximalpha.cpp ximapal.cpp ximaraw.cpp ximatga.cpp ximatran.cpp
	ximawnd.cpp tif_xfile.cpp ximaenc.cpp ximagif.cpp ximainfo.cpp
	ximajbg.cpp ximalyr.cpp ximapcx.cpp ximasel.cpp ximath.cpp ximawbmp.cpp
	xmemfile.cpp ximabmp.cpp ximaexif.cpp ximahist.cpp ximaint.cpp
	ximajpg.cpp ximamng.cpp ximapng.cpp ximaska.cpp ximatif.cpp ximawmf.cpp
)
if(WIN32)
  list(APPEND cximage_srcs CxImageDLL/CxImageCrtDll.cpp)
endif()
include_directories(../jasper/include)
add_library(cximage OBJECT ${cximage_srcs})

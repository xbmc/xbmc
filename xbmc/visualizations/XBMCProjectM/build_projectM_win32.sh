
SRC="libprojectM/SOIL.c  libprojectM/image_DXT.c  libprojectM/image_helper.c  libprojectM/stb_image_aug.c"

gcc -c -O2 -DUSE_FBO -IlibprojectM/ -I../../../lib/libSDL-WIN32/include/ -Iwin32/pthreads -I../../../visualisations/ Main.cpp libprojectM/*.cpp ${SRC}
gcc -g -s -shared -o ../../../visualisations/ProjectM_win32.vis *.o -lopengl32 -lstdc++ -lglew32 -Lwin32/glew/
rm *.o
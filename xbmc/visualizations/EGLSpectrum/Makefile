ARCH=x86_64-linux
DEFINES+=-DHAS_SDL_OPENGL -DHAS_SDL
CXXFLAGS=-fPIC
OBJS=Shader.o GUIShader.o MatrixGLES.o opengl_spectrum.o

SLIB=/Users/gimli/Devel/xbmc-ae-restart-ios//addons/visualization.glspectrum/opengl_spectrum.vis

$(SLIB): $(OBJS)
ifeq ($(findstring osx,$(ARCH)), osx)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-alias_list,/Users/gimli/Devel/xbmc-ae-restart-ios/xbmc/cores/DllLoader/exports/wrapper_mach_alias \
	-bundle -undefined dynamic_lookup -read_only_relocs suppress -o $@ \
 	/Users/gimli/Devel/xbmc-ae-restart-ios/xbmc/cores/DllLoader/exports/wrapper.o $(OBJS)
else
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -g -o $(SLIB) $(OBJS) -framework OpenGLES
endif

include /Users/gimli/Devel/xbmc-ae-restart-ios/Makefile.include


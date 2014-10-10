# Makefile
# author: Gyom / iOS-Software
# Jan 2005


###############
# Settings
macimage_name=iGoom
libobjects=../src/.libs/*.o
#buildroot=defaults read com.apple.Xcode "PBXApplicationwideBuildSettings" | cut -f2 -d"SYMROOT" | cut -f2 -d"\""
buildroot=.
BUNDLE=$(buildroot)/iGoom.bundle
APP=$(buildroot)/iGoom.app
installer=iTunes/iTunes-Installer.app


all: $(APP) $(BUNDLE)


###############
# Misc library Targets

# We build an embedable version of libgoom
StandAlone/libgoom2.0.dylib:$(libobjects)
	gcc -dynamiclib  -flat_namespace -o StandAlone/libgoom2.0.dylib $(libobjects) -install_name @executable_path/../Frameworks/libgoom2.0.dylib -compatibility_version 1 -current_version 1.0 -seg1addr 0x40000 -prebind

# We link static only with iTunes because the install_path
# cannot be set properly to embed the lib in the bundle
# We must not place it in the mac folder because the bundle will
# link against the dynamic version if it is present
iTunes/libgoom2.0.a:$(libobjects)
	libtool  -static  -o iTunes/libgoom2.0.a $(libobjects)


#########################
# Standalone Application

standalone:$(APP)

$(APP): StandAlone/libgoom2.0.dylib Makefile
	xcodebuild -target "iGoom - StandAlone" -buildstyle Deployment SYMROOT=$(buildroot)

cleanstandalone:
	xcodebuild clean -target "iGoom - StandAlone" -buildstyle Deployment SYMROOT=$(buildroot)



###############
# iTunes Plugin

itunes:$(BUNDLE)

$(BUNDLE):iTunes/libgoom2.0.a Makefile
	xcodebuild -target "iGoom - iTunes" -buildstyle Deployment SYMROOT=$(buildroot)

cleanitunes:
	xcodebuild clean -target "iGoom - iTunes" -buildstyle Deployment SYMROOT=$(buildroot)

$(installer):iTunes/Installer.applescript
	osacompile -o $(installer) -x iTunes/Installer.applescript
	


######################
# Distribution Package

all: $(APP) $(BUNDLE)

package:$(macimage_name).dmg

$(macimage_name).dmg:all ReadMe.rtf $(installer)
	rm -rf $(macimage_name).dmg $(macimage_name)
	mkdir -p $(macimage_name) 
	cp -r $(BUNDLE) $(macimage_name)
	cp -r $(APP) $(macimage_name)
	cp ReadMe.rtf $(macimage_name)
	cp -rf $(installer) $(macimage_name)/
	hdiutil create -srcfolder $(macimage_name) $(macimage_name).dmg


##########
# Clean up

clean:
	rm -rf $(BUNDLE) $(APP) $(installer)
	rm -rf $(macimage_name).dmg $(macimage_name)  $(buildroot)/iGoom.build
	rm -f StandAlone/libgoom2.* iTunes/libgoom2.* libgoom2.*


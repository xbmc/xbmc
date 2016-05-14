# WARNING: This Makefile is only be used on Kodi's own build. If you use it
# independent is cmake needed for creation!
#

### The archiver options:

AR       ?= ar
ARFLAGS  ?= ru

INCLUDES=-I. \
         -I../../../../../ \
         -I../../../../../cores/VideoPlayer/DVDDemuxers \
         -I../../../include/

DEFINES+=
CXXFLAGS=-fPIC -std=c++11
LIBNAME=libkodi.addon.sharedlibrary.api3.a

### The object files (add further files here):

OBJS=InterProcess.o \
     AddonLib.o \
     ErrorCodeNames.o \
     addon/AddonLib_Addon_General.o \
     addon/AddonLib_Addon_Network.o \
     addon/AddonLib_Addon_SoundPlay.o \
     addon/AddonLib_Addon_VFSUtils.o \
     audioengine/AddonLib_AudioEngine_General.o \
     audioengine/AddonLib_AudioEngine_Stream.o \
     gui/AddonLib_GUI_General.o \
     gui/AddonLib_GUI_ControlButton.o \
     gui/AddonLib_GUI_ControlEdit.o \
     gui/AddonLib_GUI_ControlFadeLabel.o \
     gui/AddonLib_GUI_ControlImage.o \
     gui/AddonLib_GUI_ControlLabel.o \
     gui/AddonLib_GUI_ControlProgress.o \
     gui/AddonLib_GUI_ControlRadioButton.o \
     gui/AddonLib_GUI_ControlRendering.o \
     gui/AddonLib_GUI_ControlSettingsSlider.o \
     gui/AddonLib_GUI_ControlSlider.o \
     gui/AddonLib_GUI_ControlSpin.o \
     gui/AddonLib_GUI_ControlTextBox.o \
     gui/AddonLib_GUI_DialogExtendedProgress.o \
     gui/AddonLib_GUI_DialogFileBrowser.o \
     gui/AddonLib_GUI_DialogKeyboard.o \
     gui/AddonLib_GUI_DialogNumeric.o \
     gui/AddonLib_GUI_DialogOK.o \
     gui/AddonLib_GUI_DialogProgress.o \
     gui/AddonLib_GUI_DialogSelect.o \
     gui/AddonLib_GUI_DialogTextViewer.o \
     gui/AddonLib_GUI_DialogYesNo.o \
     gui/AddonLib_GUI_ListItem.o \
     gui/AddonLib_GUI_Window.o \
     inputstream/AddonLib_InputStream.o \
     peripheral/AddonLib_Peripheral.o \
     player/AddonLib_Player_InfoTagMusic.o \
     player/AddonLib_Player_InfoTagVideo.o \
     player/AddonLib_Player_Player.o \
     player/AddonLib_Player_PlayList.o \
     pvr/AddonLib_PVR_General.o \
     pvr/AddonLib_PVR_Stream.o \
     pvr/AddonLib_PVR_Transfer.o \
     pvr/AddonLib_PVR_Trigger.o

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

all: $(LIBNAME)

### Targets:

$(LIBNAME): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

CLEAN_FILES = \
	$(LIBNAME) \

include ../../../../../../Makefile.include

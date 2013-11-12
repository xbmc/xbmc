SRCS=ISetting.cpp \
     ISettingControl.cpp \
     Setting.cpp \
     SettingCategoryAccess.cpp \
     SettingConditions.cpp \
     SettingDependency.cpp \
     SettingSection.cpp \
     SettingsManager.cpp \
     SettingRequirement.cpp \
     SettingUpdate.cpp \

LIB=settings_lib.a

include ../../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))

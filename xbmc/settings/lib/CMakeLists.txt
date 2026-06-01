set(SOURCES ISetting.cpp
            ISettingControl.cpp
            Setting.cpp
            SettingCategoryAccess.cpp
            SettingConditions.cpp
            SettingDependency.cpp
            SettingRequirement.cpp
            SettingSection.cpp
            SettingsManager.cpp
            SettingsMigration.cpp
            SettingUpdate.cpp)

set(HEADERS ISetting.h
            ISettingCallback.h
            ISettingControl.h
            ISettingControlCreator.h
            ISettingCreator.h
            ISettingsHandler.h
            ISettingsValueSerializer.h
            Setting.h
            SettingCategoryAccess.h
            SettingConditions.h
            SettingDefinitions.h
            SettingDependency.h
            SettingLevel.h
            SettingRequirement.h
            SettingSection.h
            SettingsManager.h
            SettingsMigration.h
            SettingType.h
            SettingUpdate.h)

core_add_library(settings_lib)

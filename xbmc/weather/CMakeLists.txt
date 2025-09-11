set(SOURCES GUIWindowWeather.cpp
            WeatherJob.cpp
            WeatherManager.cpp
            WeatherPropertyHelper.cpp
            WeatherTokenLocalizer.cpp)

set(HEADERS GUIWindowWeather.h
            WeatherJob.h
            WeatherManager.h
            WeatherProperties.h
            WeatherPropertyHelper.h
            WeatherTokenLocalizer.h)

core_add_library(weather)

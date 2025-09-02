set(SOURCES GUIWindowWeather.cpp
            WeatherJob.cpp
            WeatherManager.cpp
            WeatherTokenLocalizer.cpp)

set(HEADERS GUIWindowWeather.h
            WeatherJob.h
            WeatherManager.h
            WeatherTokenLocalizer.h)

core_add_library(weather)

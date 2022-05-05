set(SOURCES SpeechRecognitionAndroid.cpp
            SpeechRecognitionListenerAndroid.cpp)

set(HEADERS ISpeechRecognitionCallback.h
            SpeechRecognitionAndroid.h
            SpeechRecognitionListenerAndroid.h)

core_add_library(platform_android_speech)

set(HEADERS ISpeechRecognition.h
            ISpeechRecognitionListener.h
            SpeechRecognitionErrors.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(speech)
endif()

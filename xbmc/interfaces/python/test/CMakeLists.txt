if(TARGET ${APP_NAME_LC}::Python)
  set(SOURCES TestSwig.cpp)

  core_add_test_library(python_test)
endif()

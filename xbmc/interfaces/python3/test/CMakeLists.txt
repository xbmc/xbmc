if(PYTHON_FOUND)
  set(SOURCES TestSwig.cpp)

  core_add_test_library(python_test)
endif()

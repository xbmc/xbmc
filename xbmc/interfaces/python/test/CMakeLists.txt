if(TARGET ${APP_NAME_LC}::Python)
  set(SOURCES TestPythonBindings.cpp)

  core_add_test_library(python_test)
  if(ENABLE_STATIC_LIBS)
    target_link_libraries(python_test PRIVATE ${APP_NAME_LC}::Python)
  endif()
endif()

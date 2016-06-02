include(TestCXXAcceptsFlag)

# try to use compiler flag -std=c++11
check_cxx_accepts_flag("-std=c++11" CXX_FLAG_CXX11)
if(CXX_FLAG_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set(CXX_STD11_FLAGS "-std=c++11")
else()
  # try to use compiler flag -std=c++0x for older compilers
  check_cxx_accepts_flag("-std=c++0x" CXX_FLAG_CXX0X)
  if(CXX_FLAG_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    set(CXX_STD11_FLAGS "-std=c++0x")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CXX11 DEFAULT_MSG CXX_STD11_FLAGS)

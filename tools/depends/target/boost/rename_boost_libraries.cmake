# This script exists to strip the non-deterministic tag on static Windows
# libraries

string(REPLACE "," ";" BOOST_COMPONENTS ${BOOST_COMPONENTS_LIST})
foreach(BOOST_COMPONENT ${BOOST_COMPONENTS})
  file(GLOB BOOST_LIBRARY "${BOOST_PREFIX}/lib/libboost_${BOOST_COMPONENT}*${CMAKE_STATIC_LIBRARY_SUFFIX}")
  file(COPY_FILE "${BOOST_LIBRARY}" "${BOOST_PREFIX}/lib/libboost_${BOOST_COMPONENT}${CMAKE_STATIC_LIBRARY_SUFFIX}")
endforeach()

if(EXISTS "${INPUTFILE}")
  # if there's an input file we use it to determine which files to copy where
  file(STRINGS ${INPUTFILE} FILES)
  string(REPLACE "\n" ";" FILES "${FILES}")

  foreach(file ${FILES})
    string(REPLACE " " ";" file "${file}")
    list(GET file 0 dir)
    list(GET file 1 dest)

    list(LENGTH file deflength)
    if(deflength GREATER 2)
      list(GET file 2 move)
    endif()

    file(GLOB files ${INPUTDIR}/${dir})
    foreach(instfile ${files})
      file(COPY ${instfile} DESTINATION ${DESTDIR}/${dest})

      if(move)
        file(RENAME ${instfile} ${DESTDIR}/${move})
      endif()
    endforeach()
  endforeach()
else()
  # otherwise we assume that the content of the extracted archive is already well-formed and can just be copied
  file(COPY ${INPUTDIR}/${dir} DESTINATION ${DESTDIR})
endif()
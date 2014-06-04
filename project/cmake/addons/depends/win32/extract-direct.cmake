get_filename_component(file ${URL} NAME)
file(DOWNLOAD ${URL} ${DEST}/${file})

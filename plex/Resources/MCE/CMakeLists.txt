if(TARGET_WIN32)
  install(FILES plex-mce-icon-16x9.png
                plex-mce-icon-square.png
                plex-mce-registration-7.xml
                plex-mce-registration-vista.xml
                plex-mce-registration-xp.mcl
                plex-mce-registration-xp.xml                
          COMPONENT MCE
          DESTINATION ${BINPATH}
  )
endif(TARGET_WIN32)
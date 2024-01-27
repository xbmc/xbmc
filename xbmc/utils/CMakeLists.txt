set(SOURCES ActorProtocol.cpp
            AlarmClock.cpp
            AliasShortcutUtils.cpp
            Archive.cpp
            Base64.cpp
            BitstreamConverter.cpp
            BitstreamReader.cpp
            BitstreamStats.cpp
            BitstreamWriter.cpp
            BooleanLogic.cpp
            CharArrayParser.cpp
            CharsetConverter.cpp
            CharsetDetection.cpp
            ColorUtils.cpp
            ContentUtils.cpp
            CPUInfo.cpp
            Crc32.cpp
            CSSUtils.cpp
            DatabaseUtils.cpp
            Digest.cpp
            DiscsUtils.cpp
            EndianSwap.cpp
            EmbeddedArt.cpp
            ExecString.cpp
            FileExtensionProvider.cpp
            Fanart.cpp
            FileOperationJob.cpp
            FileUtils.cpp
            FontUtils.cpp
            GpuInfo.cpp
            GroupUtils.cpp
            HevcSei.cpp
            HTMLUtil.cpp
            HttpHeader.cpp
            HttpParser.cpp
            HttpRangeUtils.cpp
            HttpResponse.cpp
            InfoLoader.cpp
            JobManager.cpp
            JSONVariantParser.cpp
            JSONVariantWriter.cpp
            LabelFormatter.cpp
            LangCodeExpander.cpp
            LegacyPathTranslation.cpp
            Locale.cpp
            log.cpp
            Mime.cpp
            MovingSpeed.cpp
            Observer.cpp
            POUtils.cpp
            PlayerUtils.cpp
            RecentlyAddedJob.cpp
            RegExp.cpp
            rfft.cpp
            RingBuffer.cpp
            RssManager.cpp
            RssReader.cpp
            ProgressJob.cpp
            SaveFileStateJob.cpp
            ScraperParser.cpp
            ScraperUrl.cpp
            Screenshot.cpp
            SortUtils.cpp
            Speed.cpp
            StreamDetails.cpp
            StreamUtils.cpp
            StringUtils.cpp
            StringValidation.cpp
            SystemInfo.cpp
            Temperature.cpp
            TextSearch.cpp
            TimeUtils.cpp
            URIUtils.cpp
            UrlOptions.cpp
            Utf8Utils.cpp
            Variant.cpp
            VC1BitstreamParser.cpp
            Vector.cpp
            XBMCTinyXML.cpp
            XBMCTinyXML2.cpp
            XMLUtils.cpp)

set(HEADERS ActorProtocol.h
            AlarmClock.h
            AliasShortcutUtils.h
            Archive.h
            Base64.h
            BitstreamConverter.h
            BitstreamReader.h
            BitstreamStats.h
            BitstreamWriter.h
            BooleanLogic.h
            CharArrayParser.h
            CharsetConverter.h
            CharsetDetection.h
            CPUInfo.h
            ColorUtils.h
            ComponentContainer.h
            ContentUtils.h
            Crc32.h
            CSSUtils.h
            DatabaseUtils.h
            Digest.h
            DiscsUtils.h
            EndianSwap.h
            EventStream.h
            EventStreamDetail.h
            ExecString.h
            FileExtensionProvider.h
            Fanart.h
            FileOperationJob.h
            FileUtils.h
            FontUtils.h
            Geometry.h
            GlobalsHandling.h
            GpuInfo.h
            GroupUtils.h
            HDRCapabilities.h
            HevcSei.h
            HTMLUtil.h
            HttpHeader.h
            HttpParser.h
            HttpRangeUtils.h
            HttpResponse.h
            IArchivable.h
            IBufferObject.h
            ILocalizer.h
            InfoLoader.h
            IPlatformLog.h
            IRssObserver.h
            IScreenshotSurface.h
            ISerializable.h
            ISortable.h
            IXmlDeserializable.h
            Job.h
            JobManager.h
            JSONVariantParser.h
            JSONVariantWriter.h
            LabelFormatter.h
            LangCodeExpander.h
            LegacyPathTranslation.h
            Locale.h
            log.h
            logtypes.h
            Map.h
            MathUtils.h
            MemUtils.h
            Mime.h
            MovingSpeed.h
            Observer.h
            params_check_macros.h
            POUtils.h
            PlayerUtils.h
            ProgressJob.h
            RecentlyAddedJob.h
            RegExp.h
            rfft.h
            RingBuffer.h
            RssManager.h
            RssReader.h
            SaveFileStateJob.h
            ScopeGuard.h
            ScraperParser.h
            ScraperUrl.h
            Screenshot.h
            SortUtils.h
            Speed.h
            Stopwatch.h
            StreamDetails.h
            StreamUtils.h
            StringUtils.h
            StringValidation.h
            SystemInfo.h
            Temperature.h
            TextSearch.h
            TimeFormat.h
            TimeUtils.h
            TransformMatrix.h
            URIUtils.h
            UrlOptions.h
            Utf8Utils.h
            Variant.h
            VC1BitstreamParser.h
            Vector.h
            XBMCTinyXML.h
            XBMCTinyXML2.h
            XMLUtils.h
            XTimeUtils.h)

if(TARGET XSLT::XSLT)
  list(APPEND SOURCES XSLTUtils.cpp)
  list(APPEND HEADERS XSLTUtils.h)
endif()
if(TARGET EGL::EGL)
  list(APPEND SOURCES EGLUtils.cpp
                      EGLFence.cpp)
  list(APPEND HEADERS EGLUtils.h
                      EGLFence.h)
endif()

# The large map trips the clang optimizer
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set_source_files_properties(Mime.cpp PROPERTIES COMPILE_FLAGS -O0)
endif()

if(TARGET OpenGL::GL OR TARGET OpenGL::GLES)
  list(APPEND SOURCES GLUtils.cpp)
  list(APPEND HEADERS GLUtils.h)
endif()

if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND SOURCES BufferObject.cpp
                      BufferObjectFactory.cpp)
  list(APPEND HEADERS BufferObject.h
                      BufferObjectFactory.h)

  if("gbm" IN_LIST CORE_PLATFORM_NAME_LC)
    list(APPEND SOURCES DumbBufferObject.cpp)
    list(APPEND SOURCES DumbBufferObject.h)
  endif()

  if(HAVE_LINUX_MEMFD AND HAVE_LINUX_UDMABUF)
    list(APPEND SOURCES UDMABufferObject.cpp)
    list(APPEND HEADERS UDMABufferObject.h)
  endif()

  if(HAVE_LINUX_DMA_HEAP)
    list(APPEND SOURCES DMAHeapBufferObject.cpp)
    list(APPEND HEADERS DMAHeapBufferObject.h)
  endif()

  if(GBM_HAS_BO_MAP AND "gbm" IN_LIST CORE_PLATFORM_NAME_LC)
    list(APPEND SOURCES GBMBufferObject.cpp)
    list(APPEND HEADERS GBMBufferObject.h)
  endif()

  if(TARGET EGL::EGL)
    list(APPEND SOURCES EGLImage.cpp)
    list(APPEND HEADERS EGLImage.h)
  endif()

  if(TARGET LIBDRM::LIBDRM)
    list(APPEND SOURCES DRMHelpers.cpp)
    list(APPEND HEADERS DRMHelpers.h)
  endif()

  if(LIBDISPLAYINFO_FOUND)
    list(APPEND SOURCES DisplayInfo.cpp)
    list(APPEND HEADERS DisplayInfo.h)
  endif()
endif()

core_add_library(utils)

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  if(HAVE_SSE2)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse2)
  endif()
endif()

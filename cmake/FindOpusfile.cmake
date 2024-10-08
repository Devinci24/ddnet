if(NOT CMAKE_CROSSCOMPILING)
  find_package(PkgConfig QUIET)
  pkg_check_modules(PC_OPUSFILE opusfile)
endif()

set_extra_dirs_lib(OPUSFILE opus)
find_library(OPUSFILE_LIBRARY
  NAMES opusfile
  HINTS ${HINTS_OPUSFILE_LIBDIR} ${PC_OPUSFILE_LIBDIR} ${PC_OPUSFILE_LIBRARY_DIRS}
  PATHS ${PATHS_OPUSFILE_LIBDIR}
  ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
)
set_extra_dirs_include(OPUSFILE opus "${OPUSFILE_LIBRARY}")
find_path(OPUSFILE_INCLUDEDIR opusfile.h
  PATH_SUFFIXES opus
  HINTS ${HINTS_OPUSFILE_INCLUDEDIR} ${PC_OPUSFILE_INCLUDEDIR} ${PC_OPUSFILE_INCLUDE_DIRS}
  PATHS ${PATHS_OPUSFILE_INCLUDEDIR}
  ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opusfile DEFAULT_MSG OPUSFILE_LIBRARY OPUSFILE_INCLUDEDIR)

mark_as_advanced(OPUSFILE_LIBRARY OPUSFILE_INCLUDEDIR)

set(OPUSFILE_LIBRARIES ${OPUSFILE_LIBRARY})
set(OPUSFILE_INCLUDE_DIRS ${OPUSFILE_INCLUDEDIR})

if(OPUSFILE_FOUND)
  is_bundled(OPUSFILE_BUNDLED "${OPUSFILE_LIBRARY}")
  if(OPUSFILE_BUNDLED AND TARGET_OS STREQUAL "windows")
    if (TARGET_CPU_ARCHITECTURE STREQUAL "arm64")
      set(OPUSFILE_COPY_FILES
        "${EXTRA_OPUSFILE_LIBDIR}/libopusfile-0.dll"
        "${EXTRA_OPUSFILE_LIBDIR}/libopus-0.dll"
        "${EXTRA_OPUSFILE_LIBDIR}/libogg-0.dll"
        "${EXTRA_OPUSFILE_LIBDIR}/libwinpthread-1.dll"
      )
    else()
      set(OPUSFILE_COPY_FILES
        "${EXTRA_OPUSFILE_LIBDIR}/libogg.dll"
        "${EXTRA_OPUSFILE_LIBDIR}/libopus.dll"
        "${EXTRA_OPUSFILE_LIBDIR}/libopusfile.dll"
        "${EXTRA_OPUSFILE_LIBDIR}/libwinpthread-1.dll"
      )
    endif()
    if(TARGET_BITS EQUAL 32)
      list(APPEND OPUSFILE_COPY_FILES
        "${EXTRA_OPUSFILE_LIBDIR}/libgcc_s_sjlj-1.dll"
      )
    endif()
  else()
    set(OPUSFILE_COPY_FILES)
  endif()
endif()

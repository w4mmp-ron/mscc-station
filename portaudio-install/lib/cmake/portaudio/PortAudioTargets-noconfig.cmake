#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PortAudio::portaudio" for configuration ""
set_property(TARGET PortAudio::portaudio APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(PortAudio::portaudio PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libportaudio.so.19.8"
  IMPORTED_SONAME_NOCONFIG "libportaudio.so.2"
  )

list(APPEND _cmake_import_check_targets PortAudio::portaudio )
list(APPEND _cmake_import_check_files_for_PortAudio::portaudio "${_IMPORT_PREFIX}/lib/libportaudio.so.19.8" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

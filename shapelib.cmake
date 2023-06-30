set(GPSBABEL_WITH_SHAPELIB "included" CACHE STRING "no|pkgconfig|included*|custom.")
if(GPSBABEL_WITH_SHAPELIB STREQUAL "no")
  message(STATUS "shapelib disabled.")
else()
  target_compile_definitions(gpsbabel PRIVATE SHAPELIB_ENABLED)
  if(GPSBABEL_WITH_SHAPELIB STREQUAL "pkgconfig")
    message(STATUS "Using shapelib found by pkg-config.")
    find_package(PkgConfig REQUIRED)
    pkg_search_module(SHAPELIB REQUIRED shapelib IMPORTED_TARGET)
    list(APPEND LIBS PkgConfig::SHAPELIB)
    target_compile_definitions(gpsbabel PRIVATE HAVE_LIBSHAPE)
  elseif(GPSBABEL_WITH_SHAPELIB STREQUAL "included")
    add_library(shp STATIC
      shapelib/dbfopen.c
      shapelib/safileio.c
      shapelib/shpopen.c
      shapelib/shapefil.h
    )
    # note gpsbabel has conditional code include "shapelib/shapefil.h",
    # so it doesn't actually rely on the include directory being PUBLIC/INTERFACE
    target_include_directories(shp PUBLIC shape)
    target_compile_definitions(shp PRIVATE DISABLE_CVSID)
    if(MSVC)
      target_compile_definitions(shp PRIVATE _CRT_SECURE_NO_WARNINGS)
      target_compile_definitions(shp PRIVATE _CRT_NONSTDC_NO_WARNINGS)
    endif()
    # disable warning from shapelib, which we don't want to own or modify
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(shp PRIVATE -wd4267)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      # MSVC frontend clang-cl default includes /W3 which is equivalent to -Wall.
      target_compile_options(shp PRIVATE -Wno-unused-but-set-variable)
    endif()
    list(APPEND LIBS shp)
  elseif(GPSBABEL_WITH_SHAPELIB STREQUAL "custom")
    message(STATUS "shapelib is enabled but but must be manually configured.")
    message(STATUS "  e.g. -DGPSBABEL_WITH_SHAPELIB:STRING=custom -DGPSBABEL_EXTRA_LINK_DIRECTORIES:STRING=... -DGPSBABEL_EXTRA_INCLUDE_DIRECTORIES:STRING=...")
    target_compile_definitions(gpsbabel PRIVATE HAVE_LIBSHAPE)
  else()
    message(FATAL_ERROR "GPSBABEL_WITH_SHAPELIB=no|pkgconfig|included*|custom")
  endif()
endif()

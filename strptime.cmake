add_library(strptime STATIC
  strptime/strptime_l.c
  strptime/strptime.h
)
if(MSVC)
  target_compile_definitions(strptime PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(strptime PRIVATE -wd4101 -wd4102 -wd4267)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # MSVC frontend clang-cl default includes /W3 which is equivalent to -Wall.
    target_compile_options(strptime PRIVATE -Wno-unused-label -Wno-unused-variable -Wno-unused-but-set-variable)
  endif()
endif()
target_include_directories(strptime INTERFACE strptime)
list(APPEND LIBS strptime)

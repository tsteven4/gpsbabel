add_library(strptime STATIC
  strptime/strptime_l.c
  strptime/strptime.h
)
if(MSVC)
  target_compile_definitions(strptime PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
target_include_directories(strptime INTERFACE strptime)
list(APPEND LIBS strptime)

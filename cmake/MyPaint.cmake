# Narrow C-only build of the unmodified ISC-licensed libmypaint 1.6.1 sources.
include(FetchContent)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(json-c CONFIG REQUIRED)
FetchContent_Declare(mypaint
  URL https://codeload.github.com/mypaint/libmypaint/tar.gz/2768251dacce3939136c839aeca413f4aa4241d0
  URL_HASH SHA256=128b3ae94138bd3b2fc66e8d66f3664fe97afecf4e3312015758b80ddaf1be54
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(mypaint)
file(CONFIGURE OUTPUT "${mypaint_BINARY_DIR}/config.h" CONTENT "#define MYPAINT_CONFIG_USE_GLIB 0\n" @ONLY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${mypaint_SOURCE_DIR}/generate.py"
  "${mypaint_BINARY_DIR}/mypaint-brush-settings-gen.h" "${mypaint_BINARY_DIR}/brushsettings-gen.h"
  WORKING_DIRECTORY "${mypaint_SOURCE_DIR}" COMMAND_ERROR_IS_FATAL ANY)
set(mypaint_sources brushmodes.c fifo.c helpers.c mypaint-mapping.c mypaint.c
  mypaint-brush.c mypaint-brush-settings.c mypaint-fixed-tiled-surface.c mypaint-matrix.c
  mypaint-symmetry.c mypaint-rectangle.c mypaint-surface.c mypaint-tiled-surface.c
  operationqueue.c rng-double.c tilemap.c)
list(TRANSFORM mypaint_sources PREPEND "${mypaint_SOURCE_DIR}/")
add_library(opentoon_mypaint STATIC ${mypaint_sources})
set_target_properties(opentoon_mypaint PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
target_include_directories(opentoon_mypaint SYSTEM PUBLIC "${mypaint_SOURCE_DIR}" "${mypaint_BINARY_DIR}")
target_link_libraries(opentoon_mypaint PRIVATE json-c::json-c)
if(UNIX)
  target_link_libraries(opentoon_mypaint PRIVATE m)
endif()

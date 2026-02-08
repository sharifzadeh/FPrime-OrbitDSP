set(SOURCE_FILES
  OrbitDspFilter.cpp
)

set(MODULE_NAME "OrbitDspFilter")
add_library(${MODULE_NAME} STATIC ${SOURCE_FILES})
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
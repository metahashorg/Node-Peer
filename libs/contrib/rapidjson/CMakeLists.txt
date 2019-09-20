set(LIB rapidjson)

include_directories(.)

file(GLOB LIB_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

#Custom target for headers only lib
add_custom_target(${LIB} SOURCES ${LIB_HEADERS})

project(relay_server_sdk)


#if(MSVC)
#else()
#if(DEFINED BUILD_AS_LIB)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fvisibility=hidden -fvisibility-inlines-hidden -Werror")
#else()
#    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -Werror)
#endif()
#endif()
#add_definitions(-DDEBUG)

if(MSVC)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../bin)
else()
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../lib)
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/..)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../3rd/curl/include/curl)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../3rd/libuv/include)
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/../)
link_directories(${CMAKE_CURRENT_LIST_DIR}/../../lib)

file(GLOB_RECURSE SRC_LIST "*.cpp" "*.h")

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} simple_uv uv curl)

project(simple_uv)

#if(MSVC)
#else()
#if(DEFINED BUILD_AS_LIB)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=c++11 -fvisibility=hidden -fvisibility-inlines-hidden -Werror ")
#else()
#    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=c++11 -fvisibility=hidden -fvisibility-inlines-hidden -Werror )
#endif()
#endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../lib)

link_directories(${CMAKE_CURRENT_LIST_DIR}/../../lib)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../3rd/libuv/include)

file(GLOB_RECURSE SRC_LIST "*.cpp" "*.h")

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} uv)

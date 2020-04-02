cmake_minimum_required (VERSION 3.0)

project (rocketmq-proxy)

set(C_FLAGS
        -g
        -Wall
        -Wno-deprecated
        -fPIC
        -fno-strict-aliasing
        )
set(CXX_FLAGS
        -g
        -Wall
        -Wno-deprecated
        -fPIC
        -fno-strict-aliasing
        -std=c++11
        )

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
string(REPLACE ";" " " CMAKE_C_FLAGS "${C_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O0 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

include(FindPkgConfig)
pkg_search_module(GRPC REQUIRED grpc++)
foreach(_target Server Client ClientAsync ServerSync)
    add_executable(${_target} "${_target}.cc" proxy.grpc.pb.cc proxy.pb.cc)
    LINK_DIRECTORIES(${_target} /usr/local/lib )
    target_include_directories(${_target} SYSTEM PUBLIC /usr/local/include)
    target_include_directories(${_target} SYSTEM PUBLIC include)
    target_include_directories(${_target} SYSTEM PUBLIC ${GRPC_INCLUDE_DIRS})
    target_link_libraries(${_target} ${GRPC_LIBRARIES})
    target_link_libraries(${_target} rocketmq)
endforeach()

#add_subdirectory(example)
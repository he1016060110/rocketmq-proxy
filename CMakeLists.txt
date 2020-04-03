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

foreach(_target Server ProduceClient)
    add_executable(${_target} ${_target}.cpp proxy.grpc.pb.cc proxy.pb.cc)
    pkg_search_module(PROTOBUF REQUIRED protobuf)
    if(PROTOBUF_FOUND)
        include_directories(${PROTOBUF_INCLUDE_DIRS})
        target_link_libraries(${_target} ${PROTOBUF_LIBRARIES})
        target_include_directories(${_target} PRIVATE ${PROTOBUF_INCLUDE_DIR})
    endif()

    pkg_search_module(GRPCPP REQUIRED grpc++)
    if(GRPCPP_FOUND)
        include_directories(${GRPCPP_INCLUDE_DIRS})
        target_link_libraries(${_target} ${GRPCPP_LIBRARIES})
        target_include_directories(${_target} PRIVATE ${GRPCPP_INCLUDE_DIR})
    endif()
endforeach()


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
set( CMAKE_VERBOSE_MAKEFILE on )

include_directories(include)

include(FindPkgConfig)

add_executable(Server Server.cpp proxy.grpc.pb.cc proxy.pb.cc
        ConsumeCallData.cpp ConsumeCallData.cpp ProduceCallData.cpp MsgWorker.cpp ServerImpl.cpp)
add_executable(ConsumeClient ConsumeClient.cpp proxy.grpc.pb.cc proxy.pb.cc)
add_executable(ProduceClient ProduceClient.cpp proxy.grpc.pb.cc proxy.pb.cc)

foreach(_target Server ProduceClient ConsumeClient)
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

pkg_search_module(ROCKETMQ REQUIRED rocketmq)
if(ROCKETMQ_FOUND)
    include_directories(${ROCKETMQ_INCLUDE_DIRS})
    target_link_libraries(Server ${ROCKETMQ_LIBRARIES})
    target_include_directories(Server PRIVATE ${ROCKETMQ_INCLUDE_DIR})
endif()
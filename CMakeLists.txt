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
add_executable(Server Server.cpp proxy.grpc.pb.cc proxy.pb.cc)
LINK_DIRECTORIES(Server /usr/local/lib )
target_include_directories(Server SYSTEM PUBLIC /usr/local/include)
target_include_directories(Server SYSTEM PUBLIC include)
target_include_directories(Server SYSTEM PUBLIC ${GRPC_INCLUDE_DIRS})
target_link_libraries(Server ${GRPC_LIBRARIES})
target_link_libraries(Server rocketmq)

add_executable(ProduceClient ProduceClient.cpp proxy.grpc.pb.cc proxy.pb.cc)
LINK_DIRECTORIES(ProduceClient /usr/local/lib )
target_include_directories(ProduceClient SYSTEM PUBLIC /usr/local/include)
target_include_directories(ProduceClient SYSTEM PUBLIC include)
target_include_directories(ProduceClient SYSTEM PUBLIC ${GRPC_INCLUDE_DIRS})
target_link_libraries(ProduceClient ${GRPC_LIBRARIES})
cmake_minimum_required (VERSION 3.0)

project (rocketmq-proxy)

option(USE_STANDALONE_ASIO "set ON to use standalone Asio instead of Boost.Asio" OFF)
option(BUILD_TESTING "set ON to build library tests" OFF)

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

if(NOT MSVC)
    add_compile_options(-std=c++11 -Wall -Wextra -Wsign-conversion)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wthread-safety)
    endif()
else()
    add_compile_options(/W1)
endif()

add_library(simple-websocket-server INTERFACE)

target_include_directories(simple-websocket-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# TODO 2020 when Debian Jessie LTS ends:
# Remove Boost system, thread, regex components; use Boost::<component> aliases; remove Boost target_include_directories
if(USE_STANDALONE_ASIO)
    target_compile_definitions(simple-websocket-server INTERFACE USE_STANDALONE_ASIO)
    find_path(ASIO_PATH asio.hpp)
    if(NOT ASIO_PATH)
        message(FATAL_ERROR "Standalone Asio not found")
    else()
        target_include_directories(simple-websocket-server INTERFACE ${ASIO_PATH})
    endif()
else()
    find_package(Boost 1.54.0 COMPONENTS system thread coroutine context REQUIRED)
    target_link_libraries(simple-websocket-server INTERFACE ${Boost_LIBRARIES})
    target_include_directories(simple-websocket-server INTERFACE ${Boost_INCLUDE_DIR})
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        target_compile_definitions(simple-websocket-server INTERFACE USE_BOOST_REGEX)
        find_package(Boost 1.54.0 COMPONENTS regex REQUIRED)
        target_link_libraries(simple-websocket-server INTERFACE ${Boost_LIBRARIES})
        target_include_directories(simple-websocket-server INTERFACE ${Boost_INCLUDE_DIR})
    endif()
endif()
if(WIN32)
    target_link_libraries(simple-websocket-server INTERFACE ws2_32 wsock32)
endif()

if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()
find_package(OpenSSL REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${OPENSSL_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${OPENSSL_INCLUDE_DIR})

# If Simple-WebSocket-Server is not a sub-project:
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    add_executable(ProducerClient ProducerClient.cpp)
    target_link_libraries(ProducerClient simple-websocket-server)
    target_include_directories(ProducerClient PRIVATE include)
    target_link_libraries(ProducerClient rocketmq)
    target_include_directories(ProducerClient PRIVATE ${Boost_INCLUDE_DIR})

    add_executable(ConsumerClient ConsumerClient.cpp)
    target_link_libraries(ConsumerClient simple-websocket-server)
    target_include_directories(ConsumerClient PRIVATE include)
    target_link_libraries(ConsumerClient rocketmq)
    target_include_directories(ConsumerClient PRIVATE ${Boost_INCLUDE_DIR})

    add_executable(Server Server.cpp)
    target_link_libraries(Server simple-websocket-server)
    target_link_libraries(Server rocketmq)
    target_link_libraries(Server curl)
    target_include_directories(Server PRIVATE ${Boost_INCLUDE_DIR})
    target_include_directories(Server PRIVATE include)

    #set(BUILD_TESTING ON)
    install(FILES asio_compatibility.hpp server_ws.hpp client_ws.hpp server_wss.hpp client_wss.hpp crypto.hpp
            utility.hpp status_code.hpp mutex.hpp DESTINATION include)
    install(TARGETS Server DESTINATION bin)
    install(TARGETS ProducerClient DESTINATION bin)
    install(TARGETS ConsumerClient DESTINATION bin)

endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(example)
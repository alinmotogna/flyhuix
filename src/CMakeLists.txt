# NOTE: g++ 5.4 doesn't work, for some reason cannot link to boost::locale
#       so g++ forced to 4.8
set(CMAKE_CXX_COMPILER g++-4.8)

project(FlyHuiX)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native")

set(BOOST_ROOT /home/build/omm-custom-libs/boost-1_59/)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost ${BOOST_VER} REQUIRED COMPONENTS chrono system thread log_setup log filesystem locale)
find_package(Threads REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(../3rdparty/casablanca/include)

aux_source_directory(. SRC_LIST)
add_executable(flyhuix  ${SRC_LIST})

target_link_libraries( flyhuix
${CMAKE_SOURCE_DIR}/../3rdparty/casablanca/lib/libcpprest.so #cpprest
${Boost_LIBRARIES} #boost
${CMAKE_THREAD_LIBS_INIT}
)

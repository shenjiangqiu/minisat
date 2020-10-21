cmake_minimum_required(VERSION 3.10)
project(minisat)

option(REAL_CPU_TIME "option for measure REALCPUTIME" OFF)

set(SPDLOG_FMT_EXTERNAL ON)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(ZLIB REQUIRED)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES core/*.cc)
add_executable(minisat "${SOURCES}")
target_link_libraries(minisat PUBLIC minisat_utiles satacc ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} spdlog)
target_include_directories(minisat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" ${Boost_INCLUDE_DIRS} )

if(REAL_CPU_TIME)
target_compile_definitions(minisat PUBLIC REAL_CPU_TIME)
endif()
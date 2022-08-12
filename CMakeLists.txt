cmake_minimum_required(VERSION 3.10)
project(minisat)

option(REAL_CPU_TIME "option for measure REALCPUTIME" OFF)
option(HISTO "option for collect the histogram" OFF)
set(SPDLOG_FMT_EXTERNAL ON)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(ZLIB REQUIRED)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
file(GLOB SOURCES core/*.cc)
add_executable(minisat "${SOURCES}")
target_include_directories(minisat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" ${Boost_INCLUDE_DIRS} )

if(REAL_CPU_TIME)
target_compile_definitions(minisat PUBLIC REAL_CPU_TIME)
target_link_libraries(minisat PUBLIC minisat_utiles  ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} )
else()
target_link_libraries(minisat PUBLIC minisat_utiles ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} rusttools fmt)
endif()

if(HISTO)
target_compile_definitions(minisat PUBLIC HISTO)

endif()
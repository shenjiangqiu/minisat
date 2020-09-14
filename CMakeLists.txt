cmake_minimum_required(VERSION 3.10)
project(minisat)


set(SPDLOG_FMT_EXTERNAL ON)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(ZLIB REQUIRED)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES core/*.cc)
add_executable(minisat "${SOURCES}")
target_link_libraries(minisat PUBLIC minisat_utiles satacc ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} spdlog)
target_include_directories(minisat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" ${Boost_INCLUDE_DIRS} )


cmake_minimum_required(VERSION 3.10)
project(minisat)

add_subdirectory(satacc)
find_package(Boost REQUIRED COMPONENTS serialization)
find_package(ZLIB REQUIRED)

file(GLOB SOURCES core/*.cc)
add_executable(minisat "${SOURCES}")
target_link_libraries(minisat PUBLIC minisat_utiles satacc ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(minisat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
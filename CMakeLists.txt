cmake_minimum_required(VERSION 3.0.0)
project(electroslag VERSION 0.1.0)

file(GLOB_RECURSE electroslag_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "electroslag/*.cpp")
add_executable(electroslag ${electroslag_sources})
target_include_directories(electroslag PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/graphics  # For glLoadGen
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/math      # For glm
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/serialize # For RapidJSON
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/texture   # For gli
)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(electroslag PUBLIC ELECTROSLAG_BUILD=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(electroslag PUBLIC ELECTROSLAG_BUILD=2)
elseif (CMAKE_BUILD_TYPE MATCHES Ship)
    target_compile_definitions(electroslag PUBLIC ELECTROSLAG_BUILD=3)
endif()

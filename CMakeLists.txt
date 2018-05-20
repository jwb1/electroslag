CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(electroslag VERSION 0.1.0)

FILE(GLOB_RECURSE electroslag_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "electroslag/*.cpp")
ADD_EXECUTABLE(electroslag ${electroslag_sources})
TARGET_INCLUDE_DIRECTORIES(electroslag PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/graphics  # For glLoadGen
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/math      # For glm
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/serialize # For RapidJSON
    ${CMAKE_CURRENT_SOURCE_DIR}/electroslag/texture   # For gli
    $ENV{PACKAGES_ROOT}/Boost                         # For boost
)
IF (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(electroslag PUBLIC ELECTROSLAG_BUILD=1)
ELSEIF (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(electroslag PUBLIC ELECTROSLAG_BUILD=2)
ELSEIF (CMAKE_BUILD_TYPE MATCHES Ship)
    target_compile_definitions(electroslag PUBLIC ELECTROSLAG_BUILD=3)
ENDIF()

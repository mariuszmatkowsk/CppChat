project(${CMAKE_PROJECT_NAME})

add_library(ScreenState INTERFACE ScreenState.hpp)

target_include_directories(
    ScreenState
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

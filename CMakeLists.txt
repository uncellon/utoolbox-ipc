cmake_minimum_required(VERSION 3.16)
project(UTIPC VERSION 0.0.1)

option(BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(UTCore REQUIRED)

file(GLOB HEADERS include/ut/ipc/*.h)
file(GLOB SOURCES include/ut/ipc/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_library(UT::IPC ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC UT::Core)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
        PUBLIC_HEADER "${HEADERS}"
        EXPORT_NAME IPC
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/ut/ipc>
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    PUBLIC_HEADER
        DESTINATION "include/ut/ipc"
)

install(
    EXPORT ${PROJECT_NAME}Targets
    NAMESPACE UT::
    FILE "${PROJECT_NAME}Targets.cmake"
    DESTINATION "lib/cmake/${PROJECT_NAME}"
)

# Generate local "${PROJECT_NAME}Targets.cmake"
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake)
export(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
)

write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION "lib/cmake/${PROJECT_NAME}"
)

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
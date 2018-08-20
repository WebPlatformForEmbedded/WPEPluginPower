cmake_minimum_required(VERSION 2.8)

SET(COMPONENT_NAME WPEPluginPower)
add_custom_target(install-component-${COMPONENT_NAME})
add_custom_target(uninstall-component-${COMPONENT_NAME})

if(CLION_ENVIRONMENT)
    add_dependencies(install-component-${COMPONENT_NAME} install-component-WPEFrameworkPlugins)
    add_dependencies(uninstall-component-${COMPONENT_NAME} uninstall-component-WPEFrameworkPlugins)
endif()

# WPEFRAMEWORK_ROOT acts as the root directory for this project, in case it is embedded in a larger, surrounding project.
# COMPONENT_NAME is used to group multiple install targets (one per module).
# MODULE_NAME is used as a replacement for PROJECT_NAME, as using project(x) and PROJECT_NAME seems to interfere with
# Yocto builds. In some cases where PROJECT_NAME is defined, we simple assign this variable to MODULE_NAME.
# Be careful to correctly set MODULE_NAME, as it is also used in the uninstall.cmake.in files for naming the uninstall manifest.
# Additionally, install targets are linked to the MODULE_NAME and COMPONENT_NAME.

set(PLUGIN_NAME Power)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(setup_target_properties_library)
include(show_target_properties)
include(display_list)

message("Setting up ${MODULE_NAME}")

if(CMAKE_CROSSCOMPILING)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
    foreach(searchPath ${CMAKE_FIND_ROOT_PATH})
        if(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
            include(${searchPath}/usr/include/WPEFramework/cmake/config.cmake)
        endif(EXISTS "${searchPath}/usr/include/WPEFramework/cmake/config.cmake")
    endforeach()
else(CMAKE_CROSSCOMPILING)
    if(EXISTS "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake/config.cmake")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake")
        include(${CMAKE_INSTALL_PREFIX}/include/WPEFramework/cmake/config.cmake)
    else()
        if(EXISTS "${WPEFRAMEWORK_ROOT}/Source/cmake/config.cmake")
            list(APPEND CMAKE_MODULE_PATH "${WPEFRAMEWORK_ROOT}/Source/cmake")
            include(${WPEFRAMEWORK_ROOT}/Source/cmake/config.cmake)
        endif()
    endif()
endif(CMAKE_CROSSCOMPILING)

# This is to ensure that if CMAKE_INSTALL_PREFIX is set to a non-default location, pkgconfig can find the .pc file
if (NOT CMAKE_CROSSCOMPILING)
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
    set(CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CONFIG_DIR "debug" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(CONFIG_DIR "release" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
else()
    message(FATAL_ERROR "Invalid build type: " ${CMAKE_BUILD_TYPE})
endif()

if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "Build config directory: " ${CMAKE_BINARY_DIR})
endif()

if (NOT "${WPEFRAMEWORK_ROOT}" STREQUAL "")
    set(OUTPUT_BASE_DIR ${WPEFRAMEWORK_ROOT}/output)
else()
    set(OUTPUT_BASE_DIR ${CMAKE_SOURCE_DIR}/output)
endif()
if (WPEFRAMEWORK_VERBOSE_BUILD)
    message(STATUS "Output base directory: " ${OUTPUT_BASE_DIR})
endif()

set(PLUGIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
list(APPEND CMAKE_MODULE_PATH "${PLUGIN_ROOT}/cmake")

find_package(WPEFramework REQUIRED)
find_package(BcmNxServer REQUIRED)
find_package(BcmPowerManager REQUIRED)

set(PLUGIN_DEPENDENCIES_INCLUDE_DIRS)

set(PLUGIN_DEFINITIONS
    PLUGIN_POWER)

set(PLUGIN_INCLUDE_DIRS
    ${WPEFRAMEWORK_INCLUDE_DIRS}
    ${PLUGIN_DEPENDENCIES_INCLUDE_DIRS})

set(PLUGIN_OPTIONS)

set(PLUGIN_LINK_OPTIONS)

set(PLUGIN_DEPENDENCIES)

set(PLUGIN_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkPlugins}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkCore}
    ${WPEFRAMEWORK_LIBRARY_WPEFrameworkTracing}
    ${PLUGIN_DEPENDENCIES})

file(GLOB PLUGIN_INCLUDES *.h)

set(PLUGIN_SOURCES
    Module.cpp
    Power.cpp
    )

list(APPEND PLUGIN_INCLUDE_DIRS . )

if (LIBNXSERVER_FOUND)
    message("Building the Nexus....")
    list(APPEND PLUGIN_SOURCES PowerImplementation/Broadcom/PowerImplementation.cpp)
    list(APPEND PLUGIN_LIBS ${LIBNEXUS_LIBRARY} ${LIBNXCLIENT_LIBRARY})
else()
    message("Building the STUB....")
    list(APPEND PLUGIN_SOURCES PowerImplementation/Stub/PowerImplementation.cpp)
endif ()

if (BCM_PM_FOUND)
    set(POWER_LIBS -Wl,--whole-archive ${BCM_PM_LIBRARIES} -Wl,--no-whole-archive)
    list(APPEND PLUGIN_INCLUDE_DIRS ${BCM_PM_INCLUDE_DIRS})
    list(APPEND PLUGIN_LIBS ${POWER_LIBS})
endif ()

set(PLUGIN_INPUT
    ${PLUGIN_SOURCES}
    ${PLUGIN_INCLUDES}
    )

if (BUILD_REFERENCE)
  list(APPEND PLUGIN_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

set(WPEFRAMEWORK_PLUGIN_POWER_NAME "WPEFramework" CACHE STRING "Plugin name for UI")
if (WPEFRAMEWORK_VERBOSE_BUILD)
    display_list("Defines                     : " ${PLUGIN_DEFINITIONS} )
    display_list("Compiler options            : " ${PLUGIN_OPTIONS} )
    display_list("Source files                : " ${PLUGIN_SOURCES} )
    display_list("Include files               : " ${PLUGIN_INCLUDES} )
    display_list("Include dirs                : " ${PLUGIN_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${PLUGIN_LIBS} )
    display_list("Linker options              : " ${PLUGIN_LINK_OPTIONS} )
    display_list("Dependencies                : " ${PLUGIN_DEPENDENCIES} )
endif()

add_library(${MODULE_NAME} SHARED ${PLUGIN_INPUT})
target_compile_definitions(${MODULE_NAME} PRIVATE ${PLUGIN_DEFINITIONS})
target_include_directories(${MODULE_NAME} PRIVATE ${PLUGIN_INCLUDE_DIRS})
target_link_libraries(${MODULE_NAME} ${PLUGIN_LIBS})
list_to_string(PLUGIN_LINK_OPTIONS PLUGIN_LINK_OPTIONS_STRING)
if (NOT "${PLUGIN_LINK_OPTIONS_STRING}" STREQUAL "")
  set_target_properties(${MODULE_NAME} PROPERTIES LINK_FLAGS "${PLUGIN_LINK_OPTIONS_STRING}")
endif()
set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME ${MODULE_NAME})
add_dependencies(${MODULE_NAME} install-component-WPEFrameworkPlugins)

setup_target_properties_library(${MODULE_NAME})

if (WPEFRAMEWORK_VERBOSE_BUILD)
    show_target_properties(${MODULE_NAME})
endif()

# uninstall target
configure_file(
    ${PLUGIN_ROOT}/cmake/uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    IMMEDIATE @ONLY)

add_custom_target(uninstall_${MODULE_NAME}
    COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${MODULE_NAME} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    DEPENDS ${MODULE_NAME}
    COMMENT "Uninstalling ${MODULE_NAME}")

add_custom_target(install_${MODULE_NAME}
    COMMAND ${CMAKE_COMMAND} -DCOMPONENT=${MODULE_NAME} -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
    DEPENDS ${MODULE_NAME}
    COMMENT "Installing ${MODULE_NAME}")

add_dependencies(install-component-${COMPONENT_NAME} install_${MODULE_NAME})

add_dependencies(uninstall-component-${COMPONENT_NAME} uninstall_${MODULE_NAME})

install(
    TARGETS ${MODULE_NAME} DESTINATION
    ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins
    COMPONENT ${MODULE_NAME})

# default configuration
map()
    kv(locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    kv(classname ${PLUGIN_NAME})
    kv(autostart true)
    kv(precondition ___array___)
end()
ans(WPEFRAMEWORK_PLUGIN_CONFIG)

map_append(${WPEFRAMEWORK_PLUGIN_CONFIG} precondition Platform)
json_write("${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_NAME}.json" ${WPEFRAMEWORK_PLUGIN_CONFIG})

install(
    FILES ${PLUGIN_NAME}.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/plugins/
    COMPONENT ${MODULE_NAME})

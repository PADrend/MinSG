#
# This file is part of the MinSG library.
# Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
# Copyright (C) 2014-2019 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 3.1.0)
project(MinSG VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set up install directories
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKECONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/MinSG)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(SubdirectoryProperties)
include(SubdirectorySources)
include(MinSGHelper)

if(${CMAKE_VERSION} VERSION_LESS "3.11.0") 
	include(FetchContentFallback)
else()
	include(FetchContent)
endif()

add_subdirectory(Core)
add_subdirectory(Ext)
add_subdirectory(Helper)
add_subdirectory(SceneManagement)

get_property(MinSG_SOURCES GLOBAL PROPERTY MinSG_SRCS)
add_library(MinSG SHARED ${MinSG_SOURCES})

if(MSVC)
	target_compile_definitions(MinSG PRIVATE NOMINMAX)
	target_compile_definitions(MinSG PUBLIC "MINSG_DEPRECATED=__declspec(deprecated)")
	target_compile_definitions(MinSG PRIVATE "MINSGAPI=__declspec(dllexport)")
	target_compile_definitions(MinSG INTERFACE "MINSGAPI=__declspec(dllimport)")
else()
	target_compile_definitions(MinSG PUBLIC "MINSG_DEPRECATED=__attribute__((deprecated))")
	target_compile_definitions(MinSG PRIVATE "MINSGAPI=")
	target_compile_definitions(MinSG INTERFACE "MINSGAPI=")
endif()

# --- Required dependencies ---

# Dependency to Geometry
if(NOT TARGET Geometry)
	find_package(Geometry 0.3.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(MinSG LINK_PUBLIC Geometry)

# Dependency to Rendering
if(NOT TARGET Rendering)
	find_package(Rendering 0.3.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(MinSG LINK_PUBLIC Rendering)

# Dependency to Util
if(NOT TARGET Util)
	find_package(Util 0.3.0 REQUIRED NO_MODULE)
endif()
target_link_libraries(MinSG LINK_PUBLIC Util)

# Dependency to Sound
if(NOT TARGET Sound)
	find_package(Sound 0.2.0 QUIET NO_MODULE)
endif()
if(TARGET Sound)
	append_property(MINSG_COMPILE_DEFINITIONS MINSG_HAVE_LIB_SOUND)
	target_link_libraries(MinSG LINK_PUBLIC Sound)
endif()

option(MINSG_PROFILING "Defines if profiling information is generated and output in MinSG.")
if(MINSG_PROFILING)
	append_property(MINSG_COMPILE_DEFINITIONS MINSG_PROFILING)
endif()

# Dependency to OpenMP
find_package(OpenMP QUIET)
if(OPENMP_FOUND)
	set_property(TARGET MinSG APPEND_STRING PROPERTY COMPILE_FLAGS "${OpenMP_CXX_FLAGS} ")
	set_property(TARGET MinSG APPEND_STRING PROPERTY LINK_FLAGS "${OpenMP_CXX_FLAGS} ")
endif()

# Process the global property storing compile definitions
get_property(MINSG_DEFINITIONS GLOBAL PROPERTY MINSG_COMPILE_DEFINITIONS)
target_compile_definitions(MinSG PUBLIC ${MINSG_DEFINITIONS})

# Process the global property storing extended include dirs
get_property(MINSG_EXT_INCLUDE_DIR GLOBAL PROPERTY MINSG_EXT_INCLUDE_DIRS)
target_include_directories(MinSG PRIVATE ${MINSG_EXT_INCLUDE_DIR})

# Process the global property storing extended link libraries
get_property(MINSG_EXT_LIBRARY GLOBAL PROPERTY MINSG_EXT_LIBRARIES)
target_link_libraries(MinSG LINK_PRIVATE ${MINSG_EXT_LIBRARY})

# Process the global property storing dependencies
get_property(MINSG_DEPENDENCIES_SET GLOBAL PROPERTY MINSG_DEPENDENCIES SET)
if(MINSG_DEPENDENCIES_SET)
	get_property(MINSG_DEPENDENCIES GLOBAL PROPERTY MINSG_DEPENDENCIES)
	list(REMOVE_DUPLICATES MINSG_DEPENDENCIES)
	foreach(MINSG_DEPENDENCY ${MINSG_DEPENDENCIES})
		list(FIND MINSG_DEFINITIONS ${MINSG_DEPENDENCY} LIST_INDEX)
		if(LIST_INDEX EQUAL -1)
			message(SEND_ERROR "The required dependency ${MINSG_DEPENDENCY} is missing.")
		endif()
	endforeach()
endif()

# Set version of library
set_target_properties(MinSG PROPERTIES VERSION ${MinSG_VERSION}
																			 SOVERSION ${MinSG_VERSION_MAJOR}
																			 LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Install the header files
file(GLOB MINSG_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${MINSG_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MinSG COMPONENT headers)
install(DIRECTORY Core
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MinSG COMPONENT headers
	FILES_MATCHING PATTERN "*.h"
	PATTERN ".svn" EXCLUDE
	PATTERN ".git" EXCLUDE
	PATTERN "CMakeFiles" EXCLUDE
)
install(DIRECTORY Ext
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MinSG COMPONENT headers
	FILES_MATCHING PATTERN "*.h"
	PATTERN ".svn" EXCLUDE
	PATTERN ".git" EXCLUDE
	PATTERN "CMakeFiles" EXCLUDE
)
install(DIRECTORY Helper
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MinSG COMPONENT headers
	FILES_MATCHING PATTERN "*.h"
	PATTERN ".svn" EXCLUDE
	PATTERN ".git" EXCLUDE
	PATTERN "CMakeFiles" EXCLUDE
)
install(DIRECTORY SceneManagement
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MinSG COMPONENT headers
	FILES_MATCHING PATTERN "*.h"
	PATTERN ".svn" EXCLUDE
	PATTERN ".git" EXCLUDE
	PATTERN "CMakeFiles" EXCLUDE
)

# Set MinSG's data directory to the directory inside the sources
target_compile_definitions(MinSG PRIVATE COMPILE_DEFINITIONS MINSG_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# Install the data directory and rename it to MinSG
install(DIRECTORY data/
	DESTINATION ${CMAKE_INSTALL_DATADIR}/MinSG COMPONENT data
)

# Install the library
install(TARGETS MinSG EXPORT LibraryExport
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtimelibraries
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtimelibraries
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT developmentlibraries
)

# Set include directories for users of this library
target_include_directories(MinSG
													 INTERFACE
													 "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
													 "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

# Library export
install(EXPORT LibraryExport DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR} FILE MinSGTargets.cmake COMPONENT developmentlibraries)

add_subdirectory(examples)
add_subdirectory(tests)

# Create the configuration files
include(CMakePackageConfigHelpers)
configure_package_config_file(MinSGConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/MinSGConfig.cmake
															INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR}
															PATH_VARS CMAKE_INSTALL_CMAKECONFIGDIR
															NO_SET_AND_CHECK_MACRO
															NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/MinSGConfigVersion.cmake
																 VERSION ${MinSG_VERSION}
																 COMPATIBILITY SameMajorVersion)

# Install the configuration files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MinSGConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/MinSGConfigVersion.cmake
				DESTINATION ${CMAKE_INSTALL_CMAKECONFIGDIR} COMPONENT developmentlibraries)

# Packaging
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "libminsg${MinSG_VERSION_MAJOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MinSG library")
set(CPACK_PACKAGE_VENDOR "Benjamin Eikel; Claudius Jaehn; Ralf Petring; Sascha Brandt")
set(CPACK_PACKAGE_CONTACT "Benjamin Eikel <benjamin@eikel.org>")
set(CPACK_PACKAGE_VERSION_MAJOR ${MinSG_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MinSG_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${MinSG_VERSION_PATCH})
set(CPACK_PACKAGE_FILE_NAME "libMinSG")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES ON)

include(CPack)

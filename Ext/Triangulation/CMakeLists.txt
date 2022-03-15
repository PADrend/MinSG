#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
# Copyright (C) 2019 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	Delaunay2d.cpp
	Delaunay3d.cpp
	Helper.cpp
)

minsg_add_extension(MINSG_EXT_TRIANGULATION "Defines if the MinSG extension Triangulation is built (based on external library detri).")
#minsg_add_dependencies(MINSG_EXT_TRIANGULATION MINSG_HAVE_LIB_DETRI)

if(MINSG_EXT_TRIANGULATION)	
	# Dependency to Detri
	FetchContent_Declare(
		detri
		URL ${CMAKE_CURRENT_LIST_DIR}/extern/Detri_2.6.b.tar.gz
	 	URL_MD5 "5abb59a9a37b95aaeb176d12937b8418"
	)	
	FetchContent_GetProperties(detri)
	if(NOT detri_POPULATED)
		message(STATUS "Fetching detri...")
		FetchContent_Populate(detri)
		add_subdirectory(${detri_SOURCE_DIR} ${detri_BINARY_DIR} EXCLUDE_FROM_ALL)
	endif()
	minsg_link_libraries(Detri)
	minsg_include_directories(${detri_SOURCE_DIR}/basic ${detri_SOURCE_DIR}/detri ${detri_SOURCE_DIR}/lia ${detri_SOURCE_DIR}/sos)
	append_property(MINSG_COMPILE_DEFINITIONS MINSG_HAVE_LIB_DETRI)
  set_target_properties(Detri PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

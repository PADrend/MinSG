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
	AlgoSelector.cpp
	LP.cpp
	MultiAlgoGroupNode.cpp
	Region.cpp
	RegionEvaluator.cpp
	SampleContext.cpp
	SampleRegion.cpp
	SampleStorage.cpp
	SurfelRenderer.cpp
)

minsg_add_extension(MINSG_EXT_MULTIALGORENDERING "Defines if the MinSG extension Multiple Algorithm Rendering is built." ${MINSG_RECOMMENDED_EXT})
minsg_add_dependencies(MINSG_EXT_MULTIALGORENDERING MINSG_EXT_BLUE_SURFELS MINSG_EXT_COLORCUBES MINSG_EXT_SVS)

if(MINSG_EXT_MULTIALGORENDERING)	
	# Dependency to lpsolve
	FetchContent_Declare(
		lpsolve
		URL http://downloads.sourceforge.net/lpsolve/lp_solve_5.5.2.5_source.tar.gz
		URL_MD5 "3be57261fc41dd8e210f54017220d5f7"
		PATCH_COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/extern/lpsolve-CMakeLists.txt CMakeLists.txt && cp ${CMAKE_CURRENT_LIST_DIR}/extern/lpsolve-lpsolveConfig.cmake.in lpsolveConfig.cmake.in
	)	
	FetchContent_GetProperties(lpsolve)
	if(NOT lpsolve_POPULATED)
		message(STATUS "Fetching lpsolve...")
		FetchContent_Populate(lpsolve)
		add_subdirectory(${lpsolve_SOURCE_DIR} ${lpsolve_BINARY_DIR} EXCLUDE_FROM_ALL)
	endif()  
	minsg_link_libraries(lpsolve55)
	minsg_include_directories(${lpsolve_SOURCE_DIR})
	append_property(MINSG_COMPILE_DEFINITIONS MINSG_HAVE_LIB_LPSOLVE)
  set_target_properties(lpsolve55 PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

#
# This file is part of the MinSG library.
# Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 2.8.11)

option(MINSG_BUILD_TESTS "Defines if MinSGTest is built.")
if(MINSG_BUILD_TESTS)
	add_executable(MinSGTest
		MinSGTestMain.cpp
		test_automatic.cpp
		test_cost_evaluator.cpp
		test_large_scene.cpp
		test_load_scene.cpp
		test_node_memory.cpp
		test_OutOfCore.cpp
		test_simple1.cpp
		test_spherical_sampling.cpp
		test_spherical_sampling_serialization.cpp
		test_statistics.cpp
		test_valuated_region_node.cpp
		test_visibility_vector.cpp
		Viewer/ActionWrapper.cpp
		Viewer/EventHandler.cpp
		Viewer/MoveNodeHandler.cpp
	)

	if(MSVC)
		target_compile_definitions(MinSGTest PRIVATE NOMINMAX)
	endif()
	target_link_libraries(MinSGTest LINK_PRIVATE MinSG)

	if(COMPILER_SUPPORTS_CXX11)
		set_property(TARGET MinSGTest APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++11 ")
	elseif(COMPILER_SUPPORTS_CXX0X)
		set_property(TARGET MinSGTest APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++0x ")
	elseif(MSVC)
		set_property(TARGET MinSGTest APPEND_STRING PROPERTY COMPILE_FLAGS "/std:c++14 ")
	endif()

	install(TARGETS MinSGTest
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT tests
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tests
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT tests
	)

	enable_testing()
	add_test(NAME Automatic COMMAND MinSGTest --test=5)
	add_test(NAME OutOfCore COMMAND MinSGTest --test=6)
	add_test(NAME SphericalSamplingSerialization COMMAND MinSGTest --test=11)
	add_test(NAME ValuatedRegionNode COMMAND MinSGTest --test=12)
	add_test(NAME VisibilityVector COMMAND MinSGTest --test=13)
endif()

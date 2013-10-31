#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
cmake_minimum_required(VERSION 2.8.11)

add_executable(TriangleThroughput
	TriangleThroughputMain.cpp
)

target_link_libraries(TriangleThroughput LINK_PRIVATE MinSG)

if(COMPILER_SUPPORTS_CXX11)
	set_property(TARGET TriangleThroughput APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++11 ")
elseif(COMPILER_SUPPORTS_CXX0X)
	set_property(TARGET TriangleThroughput APPEND_STRING PROPERTY COMPILE_FLAGS "-std=c++0x ")
endif()

install(TARGETS TriangleThroughput
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT examples
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT examples
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT examples
)

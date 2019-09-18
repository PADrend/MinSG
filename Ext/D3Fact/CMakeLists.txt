#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	ClientUnit.cpp
	ConsoleStream.cpp
	IOStreamHandler.cpp
	Message.cpp
	MessageDispatcher.cpp
	MessageHandler.cpp
	Session.cpp
	Shape.cpp
	StreamFSProvider.cpp
	SyncHandler.cpp
	TCPHandler.cpp
	Tools.cpp
	UDPHandler.cpp
)
add_subdirectory(Utils)

minsg_add_extension(MINSG_EXT_D3FACT "Defines if the MinSG extension D3Fact is built.")
#minsg_add_dependencies(MINSG_EXT_D3FACT MINSG_HAVE_LIB_ZLIB)

if(MINSG_EXT_D3FACT)	
	# Dependency to ZLIB
	find_package(ZLIB REQUIRED)
	if(ZLIB_FOUND)
		minsg_include_directories(${ZLIB_INCLUDE_DIRS})
		minsg_link_libraries(${ZLIB_LIBRARIES})
		append_property(MINSG_COMPILE_DEFINITIONS MINSG_HAVE_LIB_ZLIB)
	endif()
endif()

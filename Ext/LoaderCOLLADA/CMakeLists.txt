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
	LoaderCOLLADA.cpp
	Writer.cpp
)
add_subdirectory(Core)
add_subdirectory(Externals)
add_subdirectory(Utils)

minsg_add_extension(MINSG_EXT_LOADERCOLLADA "Defines if the MinSG extension for loading COLLADA files is built (based on external library OpenCOLLADA).")
#minsg_add_dependencies(MINSG_EXT_LOADERCOLLADA MINSG_HAVE_LIB_OPENCOLLADA)

if(MINSG_EXT_LOADERCOLLADA)
	# Dependency to OpenCOLLADA
	find_package(OpenCOLLADA REQUIRED NO_MODULE)
	if(OpenCOLLADA_FOUND)
		append_property(MINSG_COMPILE_DEFINITIONS MINSG_HAVE_LIB_OPENCOLLADA)
		minsg_include_directories(${OPENCOLLADA_INCLUDE_DIRS})
		minsg_link_libraries(${OPENCOLLADA_LIBRARIES})
	endif()
endif()

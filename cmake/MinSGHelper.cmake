#
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This file is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this file; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# Add a list of source files to the build of MinSG
#
function(minsg_add_sources)
	add_sources(MinSG ${ARGN})
endfunction()

#
# Define a build option for a MinSG extension
#
function(minsg_add_extension extension description)
	option(${extension} ${description})
	if(${extension})
		append_property(MINSG_COMPILE_DEFINITIONS ${extension})
	endif()
endfunction()

#
# Add a list of required dependencies if the extension is enabled
#
function(minsg_add_dependencies extension)
	if(${extension})
		append_property(MINSG_DEPENDENCIES ${ARGN})
	endif()
endfunction()

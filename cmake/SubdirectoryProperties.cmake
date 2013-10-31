#
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This file is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this file; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# Append list elements to a global property
# Create the property if it does not exist.
#
function(append_property propertyName)
	# Define the <target>_COMPILE_DEFINITIONS property if necessary.
	get_property(propertyDefinedResult GLOBAL PROPERTY ${propertyName} DEFINED)
	if(NOT propertyDefinedResult)
		define_property(GLOBAL PROPERTY ${propertyName}
			BRIEF_DOCS "Global property"
			FULL_DOCS "Global list of elements")
	endif()
	# Append to global property
	set_property(GLOBAL APPEND PROPERTY ${propertyName} ${ARGN})
endfunction()

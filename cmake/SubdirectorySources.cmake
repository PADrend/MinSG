#
# Code idea posted by Michael Wild on 2010-03-16 to cmake mailing list.
# http://www.cmake.org/pipermail/cmake/2010-March/035831.html
#
# Little modifications by Benjamin Eikel on 2010-09-29.
#
function(add_sources target)
	# Define the <target>_SRCS property if necessary.
	get_property(propertyDefinedResult GLOBAL PROPERTY ${target}_SRCS DEFINED)
	if(NOT propertyDefinedResult)
		define_property(GLOBAL PROPERTY ${target}_SRCS
			BRIEF_DOCS "Sources for the ${target} target"
			FULL_DOCS "List of source files for the ${target} target")
	endif()
	# Create the list of sources (absolute paths).
	foreach(src ${ARGN})
		if(NOT IS_ABSOLUTE "${src}")
			get_filename_component(src "${src}" ABSOLUTE)
		endif()
		list(APPEND SRCS "${src}")
	endforeach()
	# append to global property
	set_property(GLOBAL APPEND PROPERTY ${target}_SRCS "${SRCS}")
endfunction()

#
# This file is part of the MinSG library extension PathTracing.
# Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	LoaderGLTF.cpp
)

# Dependency to tinygltf
FetchContent_Declare(
	tinygltf
	GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
	GIT_TAG v2.5.0
)
FetchContent_GetProperties(tinygltf)
if(NOT tinygltf_POPULATED)
		FetchContent_Populate(tinygltf)
endif()
minsg_include_directories(${tinygltf_SOURCE_DIR})

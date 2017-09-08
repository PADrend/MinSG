#
# This file is part of the MinSG library.
# Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	PathTracer.cpp
)

minsg_add_extension(MINSG_EXT_PATHTRACING "Defines if the MinSG extension PathTracing is built.")
minsg_add_dependencies(MINSG_EXT_PATHTRACING MINSG_EXT_RAYCASTING)

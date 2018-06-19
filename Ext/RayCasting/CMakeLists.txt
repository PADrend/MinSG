#
# This file is part of the MinSG library.
# Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
#
# This library is subject to the terms of the Mozilla Public License, v. 2.0.
# You should have received a copy of the MPL along with this library; see the 
# file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
#
minsg_add_sources(
	RayCaster.cpp
)

minsg_add_extension(MINSG_EXT_RAYCASTING "Defines if the MinSG extension RayCasting is built." ON)
minsg_add_extension(MINSG_EXT_RAYCASTING_PROFILING "Defines if profiling information is generated for the MinSG extension RayCasting.")
minsg_add_dependencies(MINSG_EXT_RAYCASTING MINSG_EXT_TRIANGLETREES)
minsg_add_dependencies(MINSG_EXT_RAYCASTING_PROFILING MINSG_EXT_RAYCASTING)
